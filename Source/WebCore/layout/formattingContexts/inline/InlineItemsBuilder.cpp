/*
 * Copyright (C) 2021-2023 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InlineItemsBuilder.h"

#include "FontCascade.h"
#include "InlineSoftLineBreakItem.h"
#include "RenderStyleInlines.h"
#include "StyleResolver.h"
#include "TextBreakingPositionCache.h"
#include "TextUtil.h"
#include "UnicodeBidi.h"
#include "platform/text/TextSpacing.h"
#include <wtf/Scope.h>
#include <wtf/text/TextBreakIterator.h>
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {
namespace Layout {

struct WhitespaceContent {
    size_t length { 0 };
    bool isWordSeparator { true };
};

template<typename CharacterType>
static std::optional<WhitespaceContent> moveToNextNonWhitespacePosition(std::span<const CharacterType> characters, size_t startPosition, bool preserveNewline, bool preserveTab, bool stopAtWordSeparatorBoundary)
{
    auto hasWordSeparatorCharacter = false;
    auto isWordSeparatorCharacter = false;
    auto isWhitespaceCharacter = [&](auto character) {
        // white space processing in CSS affects only the document white space characters: spaces (U+0020), tabs (U+0009), and segment breaks.
        auto isTreatedAsSpaceCharacter = character == space || (character == newlineCharacter && !preserveNewline) || (character == tabCharacter && !preserveTab);
        isWordSeparatorCharacter = isTreatedAsSpaceCharacter;
        hasWordSeparatorCharacter = hasWordSeparatorCharacter || isWordSeparatorCharacter;
        return isTreatedAsSpaceCharacter || character == tabCharacter;
    };
    auto nextNonWhiteSpacePosition = startPosition;
    while (nextNonWhiteSpacePosition < characters.size() && isWhitespaceCharacter(characters[nextNonWhiteSpacePosition])) {
        if (stopAtWordSeparatorBoundary && hasWordSeparatorCharacter && !isWordSeparatorCharacter) [[unlikely]]
            break;
        ++nextNonWhiteSpacePosition;
    }
    return nextNonWhiteSpacePosition == startPosition ? std::nullopt : std::make_optional(WhitespaceContent { nextNonWhiteSpacePosition - startPosition, hasWordSeparatorCharacter });
}

static unsigned moveToNextBreakablePosition(unsigned startPosition, CachedLineBreakIteratorFactory& lineBreakIteratorFactory, const RenderStyle& style)
{
    auto textLength = lineBreakIteratorFactory.stringView().length();
    auto startPositionForNextBreakablePosition = startPosition;
    while (startPositionForNextBreakablePosition < textLength) {
        auto nextBreakablePosition = TextUtil::findNextBreakablePosition(lineBreakIteratorFactory, startPositionForNextBreakablePosition, style);
        // Oftentimes the next breakable position comes back as the start position (most notably hyphens).
        if (nextBreakablePosition != startPosition)
            return nextBreakablePosition - startPosition;
        ++startPositionForNextBreakablePosition;
    }
    return textLength - startPosition;
}

InlineItemsBuilder::InlineItemsBuilder(InlineContentCache& inlineContentCache, const ElementBox& root, const SecurityOrigin& securityOrigin)
    : m_inlineContentCache(inlineContentCache)
    , m_root(root)
    , m_securityOrigin(securityOrigin)
{
}

void InlineItemsBuilder::build(InlineItemPosition startPosition)
{
    InlineItemList inlineItemList;
    collectInlineItems(inlineItemList, startPosition);

    if (root().writingMode().isBidiRTL() || contentRequiresVisualReordering()) {
        // FIXME: Add support for partial, yet paragraph level bidi content handling.
        breakAndComputeBidiLevels(inlineItemList);
    }

    auto& inlineItemCache = inlineContentCache().inlineItems();
    auto contentAttributes = computeContentAttributesAndInlineTextItemWidths(inlineItemList, startPosition, inlineItemCache.content());
    auto adjustInlineContentCacheWithNewInlineItems = [&] {
        ASSERT(!startPosition || startPosition.index < inlineItemCache.content().size());
        auto isPopulatedFromCache = m_textContentPopulatedFromCache && *m_textContentPopulatedFromCache ? InlineContentCache::InlineItems::IsPopulatedFromCache::Yes : InlineContentCache::InlineItems::IsPopulatedFromCache::No;
        if (!startPosition || startPosition.index >= inlineItemCache.content().size())
            return inlineItemCache.set(WTFMove(inlineItemList), contentAttributes, isPopulatedFromCache);
        inlineItemCache.replace(startPosition.index, WTFMove(inlineItemList), contentAttributes, isPopulatedFromCache);
    };
    adjustInlineContentCacheWithNewInlineItems();

#if ASSERT_ENABLED
    // Check if we've got matching inline box start/end pairs and unique inline level items (non-text, non-inline box items).
    size_t inlineBoxStart = 0;
    size_t inlineBoxEnd = 0;
    auto inlineLevelItems = HashSet<const Box*> { };
    for (auto& inlineItem : inlineContentCache().inlineItems().content()) {
        if (inlineItem.isInlineBoxStart())
            ++inlineBoxStart;
        else if (inlineItem.isInlineBoxEnd())
            ++inlineBoxEnd;
        else {
            auto hasToBeUniqueLayoutBox = inlineItem.isAtomicInlineBox() || inlineItem.isFloat() || inlineItem.isHardLineBreak();
            if (hasToBeUniqueLayoutBox)
                ASSERT(inlineLevelItems.add(&inlineItem.layoutBox()).isNewEntry);
        }
    }
    ASSERT(inlineBoxStart == inlineBoxEnd);
#endif
}

void InlineItemsBuilder::computeInlineBoxBoundaryTextSpacings(const InlineItemList& inlineItemList)
{
    ASSERT(m_hasTextAutospace);
    char32_t lastCharacterFromPreviousRun = 0;
    size_t lastCharacterDepth = 0;
    size_t currentCharacterDepth = 0;
    InlineBoxBoundaryTextSpacings spacings;
    Vector<unsigned> inlineBoxStartIndexesOnInlineItemsList;
    bool processInlineBoxBoundary = false;
    for (unsigned inlineItemIndex = 0; inlineItemIndex < inlineItemList.size(); ++inlineItemIndex) {
        auto& inlineItem = inlineItemList[inlineItemIndex];
        if (inlineItem.isInlineBoxStart()) {
            inlineBoxStartIndexesOnInlineItemsList.append(inlineItemIndex);
            ++currentCharacterDepth;
            continue;
        }
        if (inlineItem.isInlineBoxEnd()) {
            if (!currentCharacterDepth--) {
                ASSERT_NOT_REACHED();
                // Skip unbalanced inline box start/end pairs.
                processInlineBoxBoundary = false;
                currentCharacterDepth = 0;
                continue;
            }
            processInlineBoxBoundary = true;
            continue;
        }
        auto* inlineTextItem = dynamicDowncast<InlineTextItem>(inlineItem);
        if (!inlineTextItem)
            continue;

        auto start = inlineTextItem->start();
        auto length = inlineTextItem->length();
        auto& inlineTextBox = inlineTextItem->inlineTextBox();
        auto content = inlineTextBox.content().substring(start, length);
        if (!processInlineBoxBoundary || !lastCharacterFromPreviousRun) {
            lastCharacterFromPreviousRun = TextUtil::lastBaseCharacterFromText(content);
            lastCharacterDepth = currentCharacterDepth;
            processInlineBoxBoundary = false;
            continue;
        }

        size_t boundaryDepth = std::min(currentCharacterDepth, lastCharacterDepth);
        size_t inlineBoxStartOnBoundaryIndex = inlineBoxStartIndexesOnInlineItemsList.size() - 1 - (currentCharacterDepth - boundaryDepth);
        size_t boundaryIndex = inlineBoxStartIndexesOnInlineItemsList[inlineBoxStartOnBoundaryIndex];
        const RenderStyle& boundaryOwnerStyle = inlineItemList[boundaryIndex].layoutBox().parent().style();
        const TextAutospace& boundaryTextAutospace = boundaryOwnerStyle.textAutospace();
        if (!boundaryTextAutospace.isNoAutospace() && boundaryTextAutospace.shouldApplySpacing(inlineTextBox.content().characterAt(start), lastCharacterFromPreviousRun))
            spacings.add(boundaryIndex, TextAutospace::textAutospaceSize(boundaryOwnerStyle.fontCascade().primaryFont()));

        lastCharacterFromPreviousRun = TextUtil::lastBaseCharacterFromText(content);
        lastCharacterDepth = currentCharacterDepth;
        processInlineBoxBoundary = false;
    }
    if (!spacings.isEmpty())
        inlineContentCache().setInlineBoxBoundaryTextSpacings(WTFMove(spacings));
}

static inline bool isTextOrLineBreak(const Box& layoutBox)
{
    return layoutBox.isInFlow() && (layoutBox.isInlineTextBox() || (layoutBox.isLineBreakBox() && !layoutBox.isWordBreakOpportunity()));
}

static bool requiresVisualReordering(const Box& layoutBox)
{
    if (auto* inlineTextBox = dynamicDowncast<InlineTextBox>(layoutBox))
        return inlineTextBox->hasStrongDirectionalityContent();
    if (layoutBox.isInlineBox() && layoutBox.isInFlow()) {
        auto& style = layoutBox.style();
        return style.writingMode().isBidiRTL() || (style.rtlOrdering() == Order::Logical && style.unicodeBidi() != UnicodeBidi::Normal);
    }
    return false;
}

InlineItemsBuilder::LayoutQueue InlineItemsBuilder::traverseUntilDamaged(const Box& firstDamagedLayoutBox)
{
    LayoutQueue queue;

    auto appendAndCheckForDamage = [&] (auto& layoutBox) {
        queue.append(layoutBox);
        m_contentRequiresVisualReordering = m_contentRequiresVisualReordering || requiresVisualReordering(layoutBox);
        return &layoutBox == &firstDamagedLayoutBox;
    };

    if (appendAndCheckForDamage(*root().firstChild()))
        return queue;

    while (!queue.isEmpty()) {
        while (true) {
            auto layoutBox = queue.last();
            auto isInlineBoxWithInlineContent = layoutBox->isInlineBox() && !layoutBox->isInlineTextBox() && !layoutBox->isLineBreakBox() && !layoutBox->isOutOfFlowPositioned();
            if (!isInlineBoxWithInlineContent)
                break;
            auto* firstChild = downcast<ElementBox>(layoutBox).firstChild();
            if (!firstChild)
                break;
            if (appendAndCheckForDamage(*firstChild))
                return queue;
        }

        while (!queue.isEmpty()) {
            if (auto* nextSibling = queue.takeLast()->nextSibling()) {
                if (appendAndCheckForDamage(*nextSibling))
                    return queue;
                break;
            }
        }
    }
    // How did we miss the damaged box?
    ASSERT_NOT_REACHED();
    queue.append(*root().firstChild());
    return queue;
}

InlineItemsBuilder::LayoutQueue InlineItemsBuilder::initializeLayoutQueue(InlineItemPosition startPosition)
{
    auto& root = this->root();
    if (!root.firstChild()) {
        // There should always be at least one inflow child in this inline formatting context.
        ASSERT_NOT_REACHED();
        return { };
    }

    if (!startPosition)
        return { *root.firstChild() };

    // For partial layout we need to build the layout queue up to the point where the new content is in order
    // to be able to produce non-content type of trailing inline items.
    // e.g <div><span<span>text</span></span> produces
    // [inline box start][inline box start][text][inline box end][inline box end]
    // and inserting new content after text
    // <div><span><span>text more_text</span></span> should produce
    // [inline box start][inline box start][text][ ][more_text][inline box end][inline box end]
    // where we start processing the content at the new layout box and continue with whatever we have on the stack (layout queue).
    auto& existingInlineItems = inlineContentCache().inlineItems().content();
    if (startPosition.index >= existingInlineItems.size()) {
        ASSERT_NOT_REACHED();
        return { *root.firstChild() };
    }

    auto& firstDamagedLayoutBox = existingInlineItems[startPosition.index].layoutBox();
    return traverseUntilDamaged(firstDamagedLayoutBox);
}

void InlineItemsBuilder::collectInlineItems(InlineItemList& inlineItemList, InlineItemPosition startPosition)
{
    // Traverse the tree and create inline items out of inline boxes and leaf nodes. This essentially turns the tree inline structure into a flat one.
    // <span>text<span></span><img></span> -> [InlineBoxStart][InlineLevelBox][InlineBoxStart][InlineBoxEnd][InlineLevelBox][InlineBoxEnd]
    auto layoutQueue = initializeLayoutQueue(startPosition);
    auto& inlineItemCache = this->inlineContentCache().inlineItems();

    auto partialContentOffset = [&](auto& inlineTextBox) -> std::optional<size_t> {
        if (!startPosition)
            return { };
        auto& currentInlineItems = inlineItemCache.content();
        if (startPosition.index >= currentInlineItems.size()) {
            ASSERT_NOT_REACHED();
            return { };
        }
        auto& damagedInlineItem = currentInlineItems[startPosition.index];
        if (&inlineTextBox != &damagedInlineItem.layoutBox())
            return { };
        if (auto* inlineTextItem = dynamicDowncast<InlineTextItem>(damagedInlineItem))
            return inlineTextItem->start();
        if (auto* inlineSoftLineBreakItem = dynamicDowncast<InlineSoftLineBreakItem>(damagedInlineItem))
            return inlineSoftLineBreakItem->position();
        ASSERT_NOT_REACHED();
        return { };
    };

    while (!layoutQueue.isEmpty()) {
        while (true) {
            auto layoutBox = layoutQueue.last();
            auto isInlineBoxWithInlineContent = layoutBox->isInlineBox() && !layoutBox->isInlineTextBox() && !layoutBox->isLineBreakBox() && !layoutBox->isOutOfFlowPositioned();
            if (!isInlineBoxWithInlineContent)
                break;
            // This is the start of an inline box (e.g. <span>).
            handleInlineBoxStart(layoutBox, inlineItemList);
            auto& inlineBox = downcast<ElementBox>(layoutBox);
            if (!inlineBox.hasChild())
                break;
            layoutQueue.append(*inlineBox.firstChild());
        }

        while (!layoutQueue.isEmpty()) {
            auto layoutBox = layoutQueue.takeLast();
            if (layoutBox->isOutOfFlowPositioned())
                inlineItemList.append({ layoutBox, InlineItem::Type::Opaque });
            else if (auto* inlineTextBox = dynamicDowncast<InlineTextBox>(layoutBox.get()))
                handleTextContent(*inlineTextBox, inlineItemList, partialContentOffset(*inlineTextBox));
            else if (layoutBox->isAtomicInlineBox() || layoutBox->isLineBreakBox())
                handleInlineLevelBox(layoutBox, inlineItemList);
            else if (layoutBox->isInlineBox())
                handleInlineBoxEnd(layoutBox, inlineItemList);
            else if (layoutBox->isFloatingPositioned())
                inlineItemList.append({ layoutBox, InlineItem::Type::Float });
            else
                ASSERT_NOT_REACHED();

            if (auto* nextSibling = layoutBox->nextSibling()) {
                layoutQueue.append(*nextSibling);
                break;
            }
        }
    }
}

static void replaceNonPreservedNewLineAndTabCharactersAndAppend(const InlineTextBox& inlineTextBox, StringBuilder& paragraphContentBuilder)
{
    // ubidi prefers non-preserved new lines/tabs as space characters.
    ASSERT(!TextUtil::shouldPreserveNewline(inlineTextBox));
    auto textContent = inlineTextBox.content();
    auto contentLength = textContent.length();
    auto needsUnicodeHandling = !textContent.is8Bit();
    size_t nonReplacedContentStartPosition = 0;
    for (size_t position = 0; position < contentLength;) {
        // Note that because of proper code point boundary handling (see U16_NEXT), position is incremented in an unconventional way here.
        auto startPosition = position;
        auto isNewLineOrTabCharacter = [&] {
            if (needsUnicodeHandling) {
                char32_t character;
                auto characters = textContent.span16();
                U16_NEXT(characters, position, contentLength, character);
                return character == newlineCharacter || character == tabCharacter;
            }
            auto isNewLineOrTab = textContent[position] == newlineCharacter || textContent[position] == tabCharacter;
            ++position;
            return isNewLineOrTab;
        };
        if (!isNewLineOrTabCharacter())
            continue;

        if (nonReplacedContentStartPosition < startPosition)
            paragraphContentBuilder.append(StringView(textContent).substring(nonReplacedContentStartPosition, startPosition - nonReplacedContentStartPosition));
        paragraphContentBuilder.append(space);
        nonReplacedContentStartPosition = position;
    }
    if (nonReplacedContentStartPosition < contentLength)
        paragraphContentBuilder.append(StringView(textContent).right(contentLength - nonReplacedContentStartPosition));
}

struct BidiContext {
    UnicodeBidi unicodeBidi;
    bool isLeftToRightDirection { false };
    bool isBlockLevel { false };
};
using BidiContextStack = Vector<BidiContext>;

enum class EnterExitType : uint8_t {
    EnteringBlock,
    ExitingBlock,
    EnteringInlineBox,
    ExitingInlineBox
};
static inline void handleEnterExitBidiContext(StringBuilder& paragraphContentBuilder, UnicodeBidi unicodeBidi, bool isLTR, EnterExitType enterExitType, BidiContextStack& bidiContextStack)
{
    if (enterExitType == EnterExitType::ExitingInlineBox && bidiContextStack.size() == 1) {
        // Refuse to pop the initial block entry off of the stack. It indicates unbalanced InlineBoxStart/End pairs.
        ASSERT_NOT_REACHED();
        return;
    }

    auto isEnteringBidi = enterExitType == EnterExitType::EnteringBlock || enterExitType == EnterExitType::EnteringInlineBox;
    switch (unicodeBidi) {
    case UnicodeBidi::Normal:
        // The box does not open an additional level of embedding with respect to the bidirectional algorithm.
        // For inline boxes, implicit reordering works across box boundaries.
        break;
    case UnicodeBidi::Embed:
        // Isolate and embed values are enforced by default and redundant on the block level boxes.
        if (enterExitType == EnterExitType::EnteringBlock)
            break;
        paragraphContentBuilder.append(isEnteringBidi ? (isLTR ? leftToRightEmbed : rightToLeftEmbed) : popDirectionalFormatting);
        break;
    case UnicodeBidi::Override:
        paragraphContentBuilder.append(isEnteringBidi ? (isLTR ? leftToRightOverride : rightToLeftOverride) : popDirectionalFormatting);
        break;
    case UnicodeBidi::Isolate:
        // Isolate and embed values are enforced by default and redundant on the block level boxes.
        if (enterExitType == EnterExitType::EnteringBlock)
            break;
        paragraphContentBuilder.append(isEnteringBidi ? (isLTR ? leftToRightIsolate : rightToLeftIsolate) : popDirectionalIsolate);
        break;
    case UnicodeBidi::Plaintext:
        paragraphContentBuilder.append(isEnteringBidi ? firstStrongIsolate : popDirectionalIsolate);
        break;
    case UnicodeBidi::IsolateOverride:
        if (isEnteringBidi) {
            paragraphContentBuilder.append(firstStrongIsolate);
            paragraphContentBuilder.append(isLTR ? leftToRightOverride : rightToLeftOverride);
        } else {
            paragraphContentBuilder.append(popDirectionalFormatting);
            paragraphContentBuilder.append(popDirectionalIsolate);
        }
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    isEnteringBidi ? bidiContextStack.append({ unicodeBidi, isLTR, enterExitType == EnterExitType::EnteringBlock }) : bidiContextStack.removeLast();
}

static inline void unwindBidiContextStack(StringBuilder& paragraphContentBuilder, BidiContextStack& bidiContextStack, const BidiContextStack& copyOfBidiStack, size_t& blockLevelBidiContextIndex)
{
    if (bidiContextStack.isEmpty()) {
        ASSERT_NOT_REACHED();
        return;
    }
    // Unwind all the way up to the block entry.
    size_t unwindingIndex = bidiContextStack.size() - 1;
    while (unwindingIndex && !copyOfBidiStack[unwindingIndex].isBlockLevel) {
        handleEnterExitBidiContext(paragraphContentBuilder
            , copyOfBidiStack[unwindingIndex].unicodeBidi
            , copyOfBidiStack[unwindingIndex].isLeftToRightDirection
            , EnterExitType::ExitingInlineBox
            , bidiContextStack
        );
        --unwindingIndex;
    }
    blockLevelBidiContextIndex = unwindingIndex;
    // and unwind the block entries as well.
    do {
        ASSERT(copyOfBidiStack[unwindingIndex].isBlockLevel);
        handleEnterExitBidiContext(paragraphContentBuilder
            , copyOfBidiStack[unwindingIndex].unicodeBidi
            , copyOfBidiStack[unwindingIndex].isLeftToRightDirection
            , EnterExitType::ExitingBlock
            , bidiContextStack
        );
    } while (unwindingIndex--);
}

static inline void rewindBidiContextStack(StringBuilder& paragraphContentBuilder, BidiContextStack& bidiContextStack, const BidiContextStack& copyOfBidiStack, size_t blockLevelBidiContextIndex)
{
    if (copyOfBidiStack.isEmpty()) {
        ASSERT_NOT_REACHED();
        return;
    }

    for (size_t blockLevelIndex = 0; blockLevelIndex <= blockLevelBidiContextIndex; ++blockLevelIndex) {
        handleEnterExitBidiContext(paragraphContentBuilder
            , copyOfBidiStack[blockLevelIndex].unicodeBidi
            , copyOfBidiStack[blockLevelIndex].isLeftToRightDirection
            , EnterExitType::EnteringBlock
            , bidiContextStack
        );
    }

    for (size_t inlineLevelIndex = blockLevelBidiContextIndex + 1; inlineLevelIndex < copyOfBidiStack.size(); ++inlineLevelIndex) {
        handleEnterExitBidiContext(paragraphContentBuilder
            , copyOfBidiStack[inlineLevelIndex].unicodeBidi
            , copyOfBidiStack[inlineLevelIndex].isLeftToRightDirection
            , EnterExitType::EnteringInlineBox
            , bidiContextStack
        );
    }
}

using InlineItemOffsetList = Vector<std::optional<size_t>>;
static inline void handleBidiParagraphStart(StringBuilder& paragraphContentBuilder, InlineItemOffsetList& inlineItemOffsetList, BidiContextStack& bidiContextStack)
{
    // Bidi handling requires us to close all the nested bidi contexts at the end of the line triggered by forced line breaks
    // and re-open it for the content on the next line (i.e. paragraph handling).
    auto copyOfBidiStack = bidiContextStack;

    size_t blockLevelBidiContextIndex = 0;
    unwindBidiContextStack(paragraphContentBuilder, bidiContextStack, copyOfBidiStack, blockLevelBidiContextIndex);

    inlineItemOffsetList.append({ paragraphContentBuilder.length() });
    paragraphContentBuilder.append(newlineCharacter);

    rewindBidiContextStack(paragraphContentBuilder, bidiContextStack, copyOfBidiStack, blockLevelBidiContextIndex);
}

static inline void buildBidiParagraph(const RenderStyle& rootStyle, const InlineItemList& inlineItemList,  StringBuilder& paragraphContentBuilder, InlineItemOffsetList& inlineItemOffsetList)
{
    auto bidiContextStack = BidiContextStack { };
    handleEnterExitBidiContext(paragraphContentBuilder, rootStyle.unicodeBidi(), rootStyle.writingMode().isBidiLTR(), EnterExitType::EnteringBlock, bidiContextStack);
    if (rootStyle.rtlOrdering() != Order::Logical)
        handleEnterExitBidiContext(paragraphContentBuilder, UnicodeBidi::Override, rootStyle.writingMode().isBidiLTR(), EnterExitType::EnteringBlock, bidiContextStack);

    const Box* lastInlineTextBox = nullptr;
    size_t inlineTextBoxOffset = 0;
    for (size_t index = 0; index < inlineItemList.size(); ++index) {
        auto& inlineItem = inlineItemList[index];
        auto& layoutBox = inlineItem.layoutBox();

        if (inlineItem.isText() || inlineItem.isSoftLineBreak()) {
            auto* inlineTextBox = dynamicDowncast<InlineTextBox>(layoutBox);
            auto mayAppendTextContentAsOneEntry = inlineTextBox && !TextUtil::shouldPreserveNewline(*inlineTextBox);
            if (mayAppendTextContentAsOneEntry) {
                // Append the entire InlineTextBox content and keep track of individual inline item positions as we process them.
                if (lastInlineTextBox != &layoutBox) {
                    inlineTextBoxOffset = paragraphContentBuilder.length();
                    replaceNonPreservedNewLineAndTabCharactersAndAppend(*inlineTextBox, paragraphContentBuilder);
                    lastInlineTextBox = &layoutBox;
                }
                auto* inlineTextItem = dynamicDowncast<InlineTextItem>(inlineItem);
                inlineItemOffsetList.append({ inlineTextBoxOffset + (inlineTextItem ? inlineTextItem->start() : downcast<InlineSoftLineBreakItem>(inlineItem).position()) });
            } else if (auto* inlineTextItem = dynamicDowncast<InlineTextItem>(inlineItem)) {
                inlineItemOffsetList.append({ paragraphContentBuilder.length() });
                paragraphContentBuilder.append(StringView(inlineTextItem->inlineTextBox().content()).substring(inlineTextItem->start(), inlineTextItem->length()));
            } else if (is<InlineSoftLineBreakItem>(inlineItem))
                handleBidiParagraphStart(paragraphContentBuilder, inlineItemOffsetList, bidiContextStack);
            else
                ASSERT_NOT_REACHED();
        } else if (inlineItem.isAtomicInlineBox()) {
            inlineItemOffsetList.append({ paragraphContentBuilder.length() });
            paragraphContentBuilder.append(objectReplacementCharacter);
        } else if (inlineItem.isInlineBoxStartOrEnd()) {
            // https://drafts.csswg.org/css-writing-modes/#unicode-bidi
            auto& style = inlineItem.style();
            auto initiatesControlCharacter = style.rtlOrdering() == Order::Logical && style.unicodeBidi() != UnicodeBidi::Normal;
            if (!initiatesControlCharacter) {
                // Opaque items do not have position in the bidi paragraph. They inherit their bidi level from the next inline item.
                inlineItemOffsetList.append({ });
                continue;
            }
            inlineItemOffsetList.append({ paragraphContentBuilder.length() });
            auto isEnteringBidi = inlineItem.isInlineBoxStart();
            handleEnterExitBidiContext(paragraphContentBuilder
                , style.unicodeBidi()
                , style.writingMode().isBidiLTR()
                , isEnteringBidi ? EnterExitType::EnteringInlineBox : EnterExitType::ExitingInlineBox
                , bidiContextStack
            );
        } else if (inlineItem.isHardLineBreak())
            handleBidiParagraphStart(paragraphContentBuilder, inlineItemOffsetList, bidiContextStack);
        else if (inlineItem.isWordBreakOpportunity()) {
            // Soft wrap opportunity markers are opaque to bidi. 
            inlineItemOffsetList.append({ });
        } else if (inlineItem.isFloat()) {
            // Floats are not part of the inline content which make them opaque to bidi.
            inlineItemOffsetList.append({ });
        } else if (inlineItem.isOpaque()) {
            if (inlineItem.layoutBox().isOutOfFlowPositioned()) {
                // Out of flow boxes participate in inflow layout as if they were static positioned.
                inlineItemOffsetList.append({ paragraphContentBuilder.length() });
                paragraphContentBuilder.append(objectReplacementCharacter);
            } else {
                // truly opaque items are also opaque to bidi.
                inlineItemOffsetList.append({ });
            }
        } else
            ASSERT_NOT_IMPLEMENTED_YET();

    }
}

void InlineItemsBuilder::breakAndComputeBidiLevels(InlineItemList& inlineItemList)
{
    ASSERT(!inlineItemList.isEmpty());

    StringBuilder paragraphContentBuilder;
    InlineItemOffsetList inlineItemOffsets;
    inlineItemOffsets.reserveInitialCapacity(inlineItemList.size());
    buildBidiParagraph(root().style(), inlineItemList, paragraphContentBuilder, inlineItemOffsets);
    if (paragraphContentBuilder.isEmpty()) {
        // Style may trigger visual reordering even on a completely empty content.
        // e.g. <div><span style="direction:rtl"></span></div>
        // Let's not try to do bidi handling when there's no content to reorder.
        return;
    }
    auto mayNotUseBlockDirection = root().style().unicodeBidi() == UnicodeBidi::Plaintext;
    if (!contentRequiresVisualReordering() && mayNotUseBlockDirection && TextUtil::directionForTextContent(paragraphContentBuilder) == TextDirection::LTR) {
        // UnicodeBidi::Plaintext makes directionality calculated without taking parent direction property into account.
        return;
    }
    ASSERT(inlineItemOffsets.size() == inlineItemList.size());
    // 1. Setup the bidi boundary loop by calling ubidi_setPara with the paragraph text.
    // 2. Call ubidi_getLogicalRun to advance to the next bidi boundary until we hit the end of the content.
    // 3. Set the computed bidi level on the associated inline items. Split them as needed.
    UBiDi* ubidi = ubidi_open();

    auto closeUBiDiOnExit = makeScopeExit([&] {
        ubidi_close(ubidi);
    });

    UBiDiLevel rootBidiLevel = UBIDI_DEFAULT_LTR;
    bool useHeuristicBaseDirection = root().style().unicodeBidi() == UnicodeBidi::Plaintext;
    if (!useHeuristicBaseDirection)
        rootBidiLevel = root().writingMode().isBidiLTR() ? UBIDI_LTR : UBIDI_RTL;

    auto bidiContent = StringView { paragraphContentBuilder }.upconvertedCharacters();
    auto bidiContentLength = paragraphContentBuilder.length();
    UErrorCode error = U_ZERO_ERROR;
    ASSERT(bidiContentLength);
    ubidi_setPara(ubidi
        , bidiContent
        , bidiContentLength
        , rootBidiLevel
        , nullptr
        , &error);

    if (U_FAILURE(error)) {
        ASSERT_NOT_REACHED();
        return;
    }

    size_t inlineItemIndex = 0;
    auto hasSeenOpaqueItem = false;
    for (size_t currentPosition = 0; currentPosition < bidiContentLength;) {
        UBiDiLevel bidiLevel;
        int32_t endPosition = currentPosition;
        ubidi_getLogicalRun(ubidi, currentPosition, &endPosition, &bidiLevel);

        auto setBidiLevelOnRange = [&](size_t bidiEnd, auto bidiLevelForRange) {
            // We should always have inline item(s) associated with a bidi range.
            ASSERT(inlineItemIndex < inlineItemOffsets.size());
            // Start of the range is always where we left off (bidi ranges do not have gaps).
            for (; inlineItemIndex < inlineItemOffsets.size(); ++inlineItemIndex) {
                auto offset = inlineItemOffsets[inlineItemIndex];
                auto& inlineItem = inlineItemList[inlineItemIndex];
                if (!offset) {
                    // This is an opaque item. Let's post-process it.
                    hasSeenOpaqueItem = true;
                    inlineItem.setBidiLevel(bidiLevelForRange);
                    continue;
                }
                if (*offset >= bidiEnd) {
                    // This inline item is outside of the bidi range.
                    break;
                }
                inlineItem.setBidiLevel(bidiLevelForRange);
                auto* inlineTextItem = dynamicDowncast<InlineTextItem>(inlineItem);
                if (!inlineTextItem)
                    continue;
                // Check if this text item is on bidi boundary and needs splitting.
                auto endPosition = *offset + inlineTextItem->length();
                if (endPosition > bidiEnd) {
                    inlineItemList.insert(inlineItemIndex + 1, inlineTextItem->split(bidiEnd - *offset));
                    // Right side is going to be processed at the next bidi range.
                    inlineItemOffsets.insert(inlineItemIndex + 1, bidiEnd);
                    ++inlineItemIndex;
                    break;
                }
            }
        };
        setBidiLevelOnRange(endPosition, bidiLevel);
        currentPosition = endPosition;
    }

    auto setBidiLevelForOpaqueInlineItems = [&] {
        if (!hasSeenOpaqueItem)
            return;
        // Let's not confuse ubidi with non-content entries.
        // Opaque runs are excluded from the visual list (ie. only empty inline boxes should be kept around as bidi content -to figure out their visual order).
        enum class InlineBoxHasContent : bool { No, Yes };
        Vector<InlineBoxHasContent> inlineBoxContentFlagStack;
        inlineBoxContentFlagStack.reserveInitialCapacity(inlineItemList.size());
        for (auto index = inlineItemList.size(); index--;) {
            auto& inlineItem = inlineItemList[index];
            auto& style = inlineItem.style();
            auto initiatesControlCharacter = style.rtlOrdering() == Order::Logical && style.unicodeBidi() != UnicodeBidi::Normal;

            if (inlineItem.isInlineBoxStart()) {
                ASSERT(!inlineBoxContentFlagStack.isEmpty());
                if (inlineBoxContentFlagStack.takeLast() == InlineBoxHasContent::Yes) {
                    if (!initiatesControlCharacter)
                        inlineItemList[index].setBidiLevel(InlineItem::opaqueBidiLevel);
                }
                continue;
            }
            if (inlineItem.isInlineBoxEnd()) {
                inlineBoxContentFlagStack.append(InlineBoxHasContent::No);
                if (!initiatesControlCharacter)
                    inlineItem.setBidiLevel(InlineItem::opaqueBidiLevel);
                continue;
            }
            if (inlineItem.isWordBreakOpportunity()) {
                inlineItem.setBidiLevel(InlineItem::opaqueBidiLevel);
                continue;
            }

            auto isContentfulInlineItem = [&] {
                if (auto* inlineTextItem = dynamicDowncast<InlineTextItem>(inlineItem))
                    return !inlineTextItem->isWhitespace() || TextUtil::shouldPreserveSpacesAndTabs(inlineTextItem->layoutBox());
                return inlineItem.isAtomicInlineBox() || inlineItem.isLineBreak() || (inlineItem.isOpaque() && inlineItem.layoutBox().isOutOfFlowPositioned());
            };
            if (isContentfulInlineItem()) {
                // Mark the inline box stack with "content yes", when we come across a content type of inline item
                // so that we can mark the inline box as opaque and let the content drive visual ordering.
                inlineBoxContentFlagStack.fill(InlineBoxHasContent::Yes);
            }
        }
    };
    setBidiLevelForOpaqueInlineItems();
}

static inline bool canCacheMeasuredWidthOnInlineTextItem(const InlineTextBox& inlineTextBox, bool isWhitespace)
{
    // Do not cache when:
    // 1. first-line style's unique font properties may produce non-matching width values.
    // 2. position dependent content is present (preserved tab character atm).
    if (&inlineTextBox.style() != &inlineTextBox.firstLineStyle() && !inlineTextBox.style().fontCascadeEqual(inlineTextBox.firstLineStyle()))
        return false;
    if (!isWhitespace || !TextUtil::shouldPreserveSpacesAndTabs(inlineTextBox))
        return true;
    return !inlineTextBox.hasPositionDependentContentWidth();
}

static void handleTextSpacing(TextSpacing::SpacingState& spacingState, TrimmableTextSpacings& trimmableTextSpacings, const InlineTextItem& inlineTextItem, size_t inlineItemIndex)
{
    const auto& autospace = inlineTextItem.style().textAutospace();
    auto content = inlineTextItem.inlineTextBox().content().substring(inlineTextItem.start(), inlineTextItem.length());
    if (!autospace.isNoAutospace()) {
        // We need to store information about spacing added between inline text items since it needs to be trimmed during line breaking if the consecutive items are placed on different lines
        auto characterClass = TextSpacing::characterClass(content.characterAt(0));
        if (autospace.shouldApplySpacing(spacingState.lastCharacterClassFromPreviousRun, characterClass))
            trimmableTextSpacings.add(inlineItemIndex, autospace.textAutospaceSize(inlineTextItem.style().fontCascade().primaryFont()));

        auto lastCharacterFromPreviousRun = TextUtil::lastBaseCharacterFromText(content);
        spacingState.lastCharacterClassFromPreviousRun = TextSpacing::characterClass(lastCharacterFromPreviousRun);
    } else
        spacingState.lastCharacterClassFromPreviousRun = TextSpacing::CharacterClass::Undefined;
}

InlineContentCache::InlineItems::ContentAttributes InlineItemsBuilder::computeContentAttributesAndInlineTextItemWidths(InlineItemList& inlineItemList, InlineItemPosition damagePosition, const InlineItemList& damagedItemList)
{
    if (inlineItemList.isEmpty() && !damagePosition)
        return { };

    bool isTextAndForcedLineBreakOnlyContent = true;
    size_t inlineBoxCount = 0;

    auto computeContentAttributesUpToDamage = [&] {
        if (!damagePosition)
            return;
        // ContentAttributes::requiresVisualReordering is not handled here as we don't support partial layout with bidi content.
        ASSERT(damagePosition.index < damagedItemList.size());
        for (size_t index = 0; index < damagePosition.index && index < damagedItemList.size(); ++index) {
            auto& inlineItem = damagedItemList[index];
            if (inlineItem.isText())
                continue;

            if (inlineItem.isInlineBoxStart()) {
                ++inlineBoxCount;
                continue;
            }

            if (!inlineItem.isInlineBoxEnd())
                isTextAndForcedLineBreakOnlyContent = isTextAndForcedLineBreakOnlyContent && isTextOrLineBreak(inlineItem.layoutBox());
        }
    };
    computeContentAttributesUpToDamage();

    if (m_hasTextAutospace)
        computeInlineBoxBoundaryTextSpacings(inlineItemList);

    TextSpacing::SpacingState spacingState;
    TrimmableTextSpacings trimmableTextSpacings;
    auto& inlineBoxBoundaryTextSpacings = inlineContentCache().inlineBoxBoundaryTextSpacings();
    for (size_t inlineItemIndex = 0; inlineItemIndex < inlineItemList.size(); ++inlineItemIndex) {
        auto extraInlineTextSpacing = 0.f;
        auto& inlineItem = inlineItemList[inlineItemIndex];

        if (auto* inlineTextItem = dynamicDowncast<InlineTextItem>(inlineItem)) {
            auto needsMeasuring = inlineTextItem->length() && !inlineTextItem->isZeroWidthSpaceSeparator() && canCacheMeasuredWidthOnInlineTextItem(inlineTextItem->inlineTextBox(), inlineTextItem->isWhitespace());
            if (needsMeasuring) {
                auto start = inlineTextItem->start();
                if (inlineItemIndex) {
                    // Box boudary text spacing is potentially registered for inline box start items which appear logically before an inline text item
                    auto potentialInlineBoxStartIndex = inlineItemIndex - 1;
                    if (auto inlineBoxBoundaryTextSpacing = inlineBoxBoundaryTextSpacings.find(potentialInlineBoxStartIndex); inlineBoxBoundaryTextSpacing != inlineBoxBoundaryTextSpacings.end())
                        extraInlineTextSpacing = inlineBoxBoundaryTextSpacing->value;
                }
                inlineTextItem->setWidth(TextUtil::width(*inlineTextItem, inlineTextItem->style().fontCascade(), start, start + inlineTextItem->length(), { }, TextUtil::UseTrailingWhitespaceMeasuringOptimization::Yes, spacingState) + extraInlineTextSpacing);
                handleTextSpacing(spacingState, trimmableTextSpacings, *inlineTextItem, inlineItemIndex);
            }
            continue;
        }
        spacingState.lastCharacterClassFromPreviousRun = TextSpacing::CharacterClass::Undefined;

        if (inlineItem.isInlineBoxStart()) {
            ++inlineBoxCount;
            continue;
        }

        if (!inlineItem.isInlineBoxEnd())
            isTextAndForcedLineBreakOnlyContent = isTextAndForcedLineBreakOnlyContent && isTextOrLineBreak(inlineItem.layoutBox());
    }
    inlineContentCache().setTrimmableTextSpacings(WTFMove(trimmableTextSpacings));

    return { m_contentRequiresVisualReordering, isTextAndForcedLineBreakOnlyContent, m_hasTextAutospace, inlineBoxCount };
}

bool InlineItemsBuilder::buildInlineItemListForTextFromBreakingPositionsCache(const InlineTextBox& inlineTextBox, InlineItemList& inlineItemList)
{
    auto& text = inlineTextBox.content();
    auto* breakingPositions = TextBreakingPositionCache::singleton().get({ text, { inlineTextBox.style() }, m_securityOrigin.data() });
    if (!breakingPositions)
        return false;

    auto shouldPreserveNewline = TextUtil::shouldPreserveNewline(inlineTextBox);
    auto shouldPreserveSpacesAndTabs = TextUtil::shouldPreserveSpacesAndTabs(inlineTextBox);

    auto intialSize = inlineItemList.size();
    auto contentLength = text.length();
    ASSERT(contentLength);

    inlineItemList.reserveCapacity(inlineItemList.size() + breakingPositions->size());
    size_t previousPosition = 0;
    for (auto endPosition : *breakingPositions) {
        auto startPosition = std::exchange(previousPosition, endPosition);
        if (endPosition > contentLength || startPosition >= endPosition) {
            ASSERT_NOT_REACHED();
            if (inlineItemList.size() > intialSize) {
                // Revert.
                !intialSize ? inlineItemList.clear() : inlineItemList.removeAt(intialSize, inlineItemList.size() - intialSize);
            }
            return false;
        }

        auto character = text[startPosition];
        if (character == newlineCharacter && shouldPreserveNewline) {
            inlineItemList.append(InlineSoftLineBreakItem::createSoftLineBreakItem(inlineTextBox, startPosition));
            continue;
        }

        auto isWhitespaceCharacter = character == space || character == newlineCharacter || character == tabCharacter;
        if (isWhitespaceCharacter) {
            auto isWordSeparator = character != tabCharacter || !shouldPreserveSpacesAndTabs;
            inlineItemList.append(InlineTextItem::createWhitespaceItem(inlineTextBox, startPosition, endPosition - startPosition, UBIDI_DEFAULT_LTR, isWordSeparator, { }));
            continue;
        }

        ASSERT(endPosition);
        auto hasTrailingSoftHyphen = text[endPosition - 1] == softHyphen;
        inlineItemList.append(InlineTextItem::createNonWhitespaceItem(inlineTextBox, startPosition, endPosition - startPosition, UBIDI_DEFAULT_LTR, hasTrailingSoftHyphen, { }));
    }
    return true;
}

void InlineItemsBuilder::handleTextContent(const InlineTextBox& inlineTextBox, InlineItemList& inlineItemList, std::optional<size_t> partialContentOffset)
{
    auto text = inlineTextBox.content();
    auto contentLength = text.length();
    if (!contentLength)
        return inlineItemList.append(InlineTextItem::createEmptyItem(inlineTextBox));

    m_contentRequiresVisualReordering = m_contentRequiresVisualReordering || requiresVisualReordering(inlineTextBox);

    if (inlineTextBox.isCombined())
        return inlineItemList.append(InlineTextItem::createNonWhitespaceItem(inlineTextBox, { }, contentLength, UBIDI_DEFAULT_LTR, false, { }));

    if (!partialContentOffset && buildInlineItemListForTextFromBreakingPositionsCache(inlineTextBox, inlineItemList)) {
        if (!m_textContentPopulatedFromCache)
            m_textContentPopulatedFromCache = true;
        return;
    }

    m_textContentPopulatedFromCache = false;
    auto& style = inlineTextBox.style();
    auto shouldPreserveSpacesAndTabs = TextUtil::shouldPreserveSpacesAndTabs(inlineTextBox);
    auto shouldPreserveNewline = TextUtil::shouldPreserveNewline(inlineTextBox);
    auto lineBreakIteratorFactory = CachedLineBreakIteratorFactory { text, style.computedLocale(), TextUtil::lineBreakIteratorMode(style.lineBreak()), TextUtil::contentAnalysis(style.wordBreak()) };
    auto currentPosition = partialContentOffset.value_or(0lu);
    ASSERT(currentPosition <= contentLength);

    auto handleSegmentBreak = [&] {
        // Segment breaks with preserve new line style (white-space: pre, pre-wrap, break-spaces and pre-line) compute to forced line break.
        auto isSegmentBreakCandidate = text[currentPosition] == newlineCharacter;
        if (!isSegmentBreakCandidate || !shouldPreserveNewline)
            return false;
        inlineItemList.append(InlineSoftLineBreakItem::createSoftLineBreakItem(inlineTextBox, currentPosition++));
        return true;
    };
    auto handleWhitespace = [&] {
        auto stopAtWordSeparatorBoundary = shouldPreserveSpacesAndTabs && style.fontCascade().wordSpacing();
        auto whitespaceContent = text.is8Bit()
            ? moveToNextNonWhitespacePosition(text.span8(), currentPosition, shouldPreserveNewline, shouldPreserveSpacesAndTabs, stopAtWordSeparatorBoundary)
            : moveToNextNonWhitespacePosition(text.span16(), currentPosition, shouldPreserveNewline, shouldPreserveSpacesAndTabs, stopAtWordSeparatorBoundary);
        if (!whitespaceContent)
            return false;

        ASSERT(whitespaceContent->length);
        if (style.whiteSpaceCollapse() == WhiteSpaceCollapse::BreakSpaces) {
            // https://www.w3.org/TR/css-text-3/#white-space-phase-1
            // For break-spaces, a soft wrap opportunity exists after every space and every tab.
            // FIXME: if this turns out to be a perf hit with too many individual whitespace inline items, we should transition this logic to line breaking.
            inlineItemList.appendUsingFunctor(whitespaceContent->length, [&](size_t offset) {
                return InlineTextItem::createWhitespaceItem(inlineTextBox, currentPosition + offset, 1, UBIDI_DEFAULT_LTR, whitespaceContent->isWordSeparator, { });
            });
        } else
            inlineItemList.append(InlineTextItem::createWhitespaceItem(inlineTextBox, currentPosition, whitespaceContent->length, UBIDI_DEFAULT_LTR, whitespaceContent->isWordSeparator, { }));
        currentPosition += whitespaceContent->length;
        return true;

    };
    auto handleNonBreakingSpace = [&] {
        if (style.nbspMode() != NBSPMode::Space) {
            // Let's just defer to regular non-whitespace inline items when non breaking space needs no special handling.
            return false;
        }
        auto startPosition = currentPosition;
        auto endPosition = startPosition;
        for (; endPosition < contentLength; ++endPosition) {
            if (text[endPosition] != noBreakSpace)
                break;
        }
        if (startPosition == endPosition)
            return false;
        inlineItemList.appendUsingFunctor(endPosition - startPosition, [&](size_t offset) {
            return InlineTextItem::createNonWhitespaceItem(inlineTextBox, startPosition + offset, 1, UBIDI_DEFAULT_LTR, { }, { });
        });
        currentPosition = endPosition;
        return true;
    };
    auto handleNonWhitespace = [&] {
        auto startPosition = currentPosition;
        auto endPosition = startPosition;
        auto hasTrailingSoftHyphen = false;
        if (style.hyphens() == Hyphens::None) {
            // Let's merge candidate InlineTextItems separated by soft hyphen when the style says so.
            do {
                endPosition += moveToNextBreakablePosition(endPosition, lineBreakIteratorFactory, style);
                ASSERT(startPosition < endPosition);
            } while (endPosition < contentLength && text[endPosition - 1] == softHyphen);
        } else {
            endPosition += moveToNextBreakablePosition(startPosition, lineBreakIteratorFactory, style);
            ASSERT(startPosition < endPosition);
            hasTrailingSoftHyphen = text[endPosition - 1] == softHyphen;
        }
        ASSERT_IMPLIES(style.hyphens() == Hyphens::None, !hasTrailingSoftHyphen);
        inlineItemList.append(InlineTextItem::createNonWhitespaceItem(inlineTextBox, startPosition, endPosition - startPosition, UBIDI_DEFAULT_LTR, hasTrailingSoftHyphen, { }));
        currentPosition = endPosition;
        return true;
    };
    while (currentPosition < contentLength) {
        if (handleSegmentBreak())
            continue;
        if (handleWhitespace())
            continue;
        if (handleNonBreakingSpace())
            continue;
        if (handleNonWhitespace())
            continue;
        // Unsupported content?
        ASSERT_NOT_REACHED();
    }
}

void InlineItemsBuilder::handleInlineBoxStart(const Box& inlineBox, InlineItemList& inlineItemList)
{
    inlineItemList.append({ inlineBox, InlineItem::Type::InlineBoxStart });
    m_contentRequiresVisualReordering |= requiresVisualReordering(inlineBox);
    m_hasTextAutospace |= !inlineBox.style().textAutospace().isNoAutospace();
}

void InlineItemsBuilder::handleInlineBoxEnd(const Box& inlineBox, InlineItemList& inlineItemList)
{
    inlineItemList.append({ inlineBox, InlineItem::Type::InlineBoxEnd });
    // Inline box end item itself can not trigger bidi content.
    ASSERT(contentRequiresVisualReordering() || inlineBox.writingMode().isBidiLTR() || inlineBox.style().rtlOrdering() == Order::Visual || inlineBox.style().unicodeBidi() == UnicodeBidi::Normal);
}

void InlineItemsBuilder::handleInlineLevelBox(const Box& layoutBox, InlineItemList& inlineItemList)
{
    if (layoutBox.isRubyAnnotationBox())
        return inlineItemList.append({ layoutBox, InlineItem::Type::Opaque });

    if (layoutBox.isAtomicInlineBox())
        return inlineItemList.append({ layoutBox, InlineItem::Type::AtomicInlineBox });

    if (layoutBox.isLineBreakBox())
        return inlineItemList.append({ layoutBox, layoutBox.isWordBreakOpportunity() ? InlineItem::Type::WordBreakOpportunity : InlineItem::Type::HardLineBreak });

    ASSERT_NOT_REACHED();
}

void InlineItemsBuilder::populateBreakingPositionCache(const InlineItemList& inlineItemList, const Document& document)
{
    if (inlineItemList.size() < TextBreakingPositionCache::minimumRequiredContentBreaks)
        return;

    auto inlineTextBoxContentSpan = [](const InlineItemList& inlineItemList, size_t index, const InlineTextBox* inlineTextBox) ALWAYS_INLINE_LAMBDA {
        size_t length = 0;
        for (auto& item : inlineItemList.subspan(index)) {
            if (&item.layoutBox() != inlineTextBox)
                break;
            ++length;
        }
        return inlineItemList.subspan(index, length);
    };

    // Preserve breaking positions across content mutation.
    auto& securityOrigin = document.securityOrigin();
    auto& breakingPositionCache = TextBreakingPositionCache::singleton();
    size_t index = 0;
    while (index < inlineItemList.size()) {
        auto* inlineTextBox = dynamicDowncast<InlineTextBox>(inlineItemList[index].layoutBox());
        if (!inlineTextBox) {
            ++index;
            continue;
        }

        auto span = inlineTextBoxContentSpan(inlineItemList, index, inlineTextBox);
        if (span.size() < TextBreakingPositionCache::minimumRequiredContentBreaks) {
            // Inline text box content's span is too short.
            index += span.size();
            continue;
        }

        auto isInlineTextBoxEligibleForBreakingPositionCache = inlineTextBox->content().length() >= TextBreakingPositionCache::minimumRequiredTextLengthForContentBreakCache;
        if (!isInlineTextBoxEligibleForBreakingPositionCache) {
            // Text is too short.
            index += span.size();
            continue;
        }

        TextBreakingPositionContext context { inlineTextBox->style() };
        if (breakingPositionCache.get({ inlineTextBox->content(), context, securityOrigin.data() })) {
            // Cache is already populated.
            index += span.size();
            continue;
        }

        TextBreakingPositionCache::List breakingPositionList;
        breakingPositionList.reserveInitialCapacity(span.size());
        for (auto& inlineItem : span) {
            if (auto* inlineTextItem = dynamicDowncast<InlineTextItem>(inlineItem))
                breakingPositionList.append(inlineTextItem->end());
            else if (auto* softLineBreakItem = dynamicDowncast<InlineSoftLineBreakItem>(inlineItem))
                breakingPositionList.append(softLineBreakItem->position() + 1);
            else {
                ASSERT_NOT_REACHED();
                breakingPositionList.clear();
                break;
            }
        }

        ASSERT(!breakingPositionList.isEmpty());
        if (breakingPositionList.size() >= TextBreakingPositionCache::minimumRequiredContentBreaks)
            breakingPositionCache.set({ inlineTextBox->content(), context, securityOrigin.data() }, WTFMove(breakingPositionList));
        index += span.size();
    }
}

}
}

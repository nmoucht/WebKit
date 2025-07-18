/*
 * Copyright (C) 2019-2025 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HTMLDialogElement.h"

#include "ContainerNodeInlines.h"
#include "CSSSelector.h"
#include "DocumentInlines.h"
#include "EventLoop.h"
#include "EventNames.h"
#include "FocusOptions.h"
#include "HTMLButtonElement.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "Logging.h"
#include "NodeInlines.h"
#include "PopoverData.h"
#include "PseudoClassChangeInvalidation.h"
#include "RenderBlock.h"
#include "RenderElement.h"
#include "ScopedEventQueue.h"
#include "ToggleEvent.h"
#include "ToggleEventTask.h"
#include "TypedElementDescendantIteratorInlines.h"
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

WTF_MAKE_TZONE_OR_ISO_ALLOCATED_IMPL(HTMLDialogElement);

using namespace HTMLNames;

HTMLDialogElement::HTMLDialogElement(const QualifiedName& tagName, Document& document)
    : HTMLElement(tagName, document)
{
}

ExceptionOr<void> HTMLDialogElement::show()
{
    // If the element already has an open attribute, then return.
    if (isOpen()) {
        if (!isModal())
            return { };
        return Exception { ExceptionCode::InvalidStateError, "Cannot call show() on an open modal dialog."_s };
    }

    Ref event = ToggleEvent::create(eventNames().beforetoggleEvent, { EventInit { }, "closed"_s, "open"_s }, Event::IsCancelable::Yes);
    dispatchEvent(event);
    if (event->defaultPrevented())
        return { };

    if (isOpen())
        return { };

    queueDialogToggleEventTask(ToggleState::Closed, ToggleState::Open);

    setAttributeWithoutSynchronization(openAttr, emptyAtom());

    Ref document = this->document();
    m_previouslyFocusedElement = document->focusedElement();

    RefPtr hideUntil = topmostPopoverAncestor(TopLayerElementType::Other);

    document->hideAllPopoversUntil(hideUntil.get(), FocusPreviousElement::No, FireEvents::No);

    runFocusingSteps();

    return { };
}

ExceptionOr<void> HTMLDialogElement::showModal()
{
    // If subject already has an open attribute, then throw an "InvalidStateError" DOMException.
    if (isOpen()) {
        if (isModal())
            return { };
        return Exception { ExceptionCode::InvalidStateError, "Cannot call showModal() on an open non-modal dialog."_s };
    }

    // If subject is not connected, then throw an "InvalidStateError" DOMException.
    if (!isConnected())
        return Exception { ExceptionCode::InvalidStateError, "Element is not connected."_s };

    if (isPopoverShowing())
        return Exception { ExceptionCode::InvalidStateError, "Element is already an open popover."_s };

    Ref document = this->document();
    if (!document->isFullyActive())
        return Exception { ExceptionCode::InvalidStateError, "Invalid for dialogs within documents that are not fully active."_s };

    Ref event = ToggleEvent::create(eventNames().beforetoggleEvent, { EventInit { }, "closed"_s, "open"_s }, Event::IsCancelable::Yes);
    dispatchEvent(event);
    if (event->defaultPrevented())
        return { };

    if (isOpen())
        return { };

    if (!isConnected())
        return { };

    if (isPopoverShowing())
        return { };

    queueDialogToggleEventTask(ToggleState::Closed, ToggleState::Open);

    // setAttributeWihoutSynchronization will dispatch a DOMSubtreeModified event.
    // Postpone callback execution that can potentially make the dialog disconnected.
    EventQueueScope scope;
    setAttributeWithoutSynchronization(openAttr, emptyAtom());

    setIsModal(true);

    auto containingBlockBeforeStyleResolution = SingleThreadWeakPtr<RenderBlock> { };
    if (auto* renderer = this->renderer())
        containingBlockBeforeStyleResolution = renderer->containingBlock();

    if (!isInTopLayer())
        addToTopLayer();

    RenderElement::markRendererDirtyAfterTopLayerChange(this->checkedRenderer().get(), containingBlockBeforeStyleResolution.get());

    m_previouslyFocusedElement = document->focusedElement();

    RefPtr hideUntil = topmostPopoverAncestor(TopLayerElementType::Other);

    document->hideAllPopoversUntil(hideUntil.get(), FocusPreviousElement::No, FireEvents::No);

    runFocusingSteps();

    return { };
}

void HTMLDialogElement::close(const String& result)
{
    if (!isOpen())
        return;

    Ref event = ToggleEvent::create(eventNames().beforetoggleEvent, { EventInit { }, "open"_s, "closed"_s }, Event::IsCancelable::No);
    dispatchEvent(event);

    if (!isOpen())
        return;

    queueDialogToggleEventTask(ToggleState::Open, ToggleState::Closed);

    removeAttribute(openAttr);

    if (isModal())
        removeFromTopLayer();

    setIsModal(false);

    if (!result.isNull())
        m_returnValue = result;

    if (RefPtr element = std::exchange(m_previouslyFocusedElement, nullptr).get()) {
        FocusOptions options;
        options.preventScroll = true;
        element->focus(options);
    }

    queueTaskToDispatchEvent(TaskSource::UserInteraction, Event::create(eventNames().closeEvent, Event::CanBubble::No, Event::IsCancelable::No));
}

void HTMLDialogElement::requestClose(const String& returnValue)
{
    if (!isOpen())
        return;

    auto cancelEvent = Event::create(eventNames().cancelEvent, Event::CanBubble::No, Event::IsCancelable::Yes);
    dispatchEvent(cancelEvent);
    if (!cancelEvent->defaultPrevented())
        close(returnValue);
}

bool HTMLDialogElement::isValidCommandType(const CommandType command)
{
    return HTMLElement::isValidCommandType(command) || command == CommandType::ShowModal || command == CommandType::Close
        || command == CommandType::RequestClose;
}

bool HTMLDialogElement::handleCommandInternal(HTMLButtonElement& invoker, const CommandType& command)
{
    if (HTMLElement::handleCommandInternal(invoker, command))
        return true;

    if (isPopoverShowing())
        return false;

    if (isOpen()) {
        if (command == CommandType::Close) {
            close(invoker.value().string());
            return true;
        }
        if (command == CommandType::RequestClose) {
            requestClose(invoker.value().string());
            return true;
        }
    } else {
        if (command == CommandType::ShowModal) {
            showModal();
            return true;
        }
    }

    return false;
}

void HTMLDialogElement::queueCancelTask()
{
    queueTaskKeepingThisNodeAlive(TaskSource::UserInteraction, [weakThis = WeakPtr { *this }] {
        RefPtr protectedThis = weakThis.get();
        if (!protectedThis)
            return;
        auto cancelEvent = Event::create(eventNames().cancelEvent, Event::CanBubble::No, Event::IsCancelable::Yes);
        protectedThis->dispatchEvent(cancelEvent);
        if (!cancelEvent->defaultPrevented())
            protectedThis->close(nullString());
    });
}

// https://html.spec.whatwg.org/multipage/interactive-elements.html#dialog-focusing-steps
void HTMLDialogElement::runFocusingSteps()
{
    RefPtr<Element> control;
    if (hasAttributeWithoutSynchronization(HTMLNames::autofocusAttr))
        control = this;
    if (!control)
        control = findFocusDelegate();

    if (!control)
        control = this;

    Ref controlDocument = control->document();
    RefPtr page = controlDocument->page();
    if (!page)
        return;

    if (control->isFocusable())
        control->runFocusingStepsForAutofocus();
    else if (m_isModal)
        protectedDocument()->setFocusedElement(nullptr); // Focus fixup rule

    RefPtr topDocument = controlDocument->sameOriginTopLevelTraversable();
    if (!topDocument)
        return;

    topDocument->clearAutofocusCandidates();
    page->setAutofocusProcessed();
}

bool HTMLDialogElement::supportsFocus() const
{
    return true;
}

void HTMLDialogElement::removedFromAncestor(RemovalType removalType, ContainerNode& oldParentOfRemovedTree)
{
    HTMLElement::removedFromAncestor(removalType, oldParentOfRemovedTree);
    setIsModal(false);
}

void HTMLDialogElement::setIsModal(bool newValue)
{
    if (m_isModal == newValue)
        return;
    Style::PseudoClassChangeInvalidation styleInvalidation(*this, CSSSelector::PseudoClass::Modal, newValue);
    m_isModal = newValue;
}

void HTMLDialogElement::queueDialogToggleEventTask(ToggleState oldState, ToggleState newState)
{
    if (!m_toggleEventTask)
        m_toggleEventTask = ToggleEventTask::create(*this);

    RefPtr { m_toggleEventTask }->queue(oldState, newState);
}

bool HTMLDialogElement::isOpen() const
{
    return hasAttributeWithoutSynchronization(HTMLNames::openAttr);
}

}

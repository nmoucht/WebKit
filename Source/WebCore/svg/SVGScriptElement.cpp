/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2018 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "SVGScriptElement.h"

#include "Document.h"
#include "DocumentInlines.h"
#include "Event.h"
#include "NodeInlines.h"
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

WTF_MAKE_TZONE_OR_ISO_ALLOCATED_IMPL(SVGScriptElement);

inline SVGScriptElement::SVGScriptElement(const QualifiedName& tagName, Document& document, bool wasInsertedByParser, bool alreadyStarted)
    : SVGElement(tagName, document, makeUniqueRef<PropertyRegistry>(*this))
    , SVGURIReference(this)
    , ScriptElement(*this, wasInsertedByParser, alreadyStarted)
    , m_loadEventTimer(*this, &SVGElement::loadEventTimerFired)
{
    ASSERT(hasTagName(SVGNames::scriptTag));
}

Ref<SVGScriptElement> SVGScriptElement::create(const QualifiedName& tagName, Document& document, bool insertedByParser)
{
    return adoptRef(*new SVGScriptElement(tagName, document, insertedByParser, false));
}

void SVGScriptElement::attributeChanged(const QualifiedName& name, const AtomString& oldValue, const AtomString& newValue, AttributeModificationReason attributeModificationReason)
{
    SVGURIReference::parseAttribute(name, newValue);
    SVGElement::attributeChanged(name, oldValue, newValue, attributeModificationReason);
}

void SVGScriptElement::svgAttributeChanged(const QualifiedName& attrName)
{
    InstanceInvalidationGuard guard(*this);

    if (SVGURIReference::isKnownAttribute(attrName)) {
        handleSourceAttribute(href());
        return;
    }

    SVGElement::svgAttributeChanged(attrName);
}

Node::InsertedIntoAncestorResult SVGScriptElement::insertedIntoAncestor(InsertionType insertionType, ContainerNode& parentOfInsertedTree)
{
    auto result1 = SVGElement::insertedIntoAncestor(insertionType, parentOfInsertedTree);
    auto result2 = ScriptElement::insertedIntoAncestor(insertionType, parentOfInsertedTree);
    return result1 == InsertedIntoAncestorResult::NeedsPostInsertionCallback ? result1 : result2;
}

void SVGScriptElement::didFinishInsertingNode()
{
    ScriptElement::didFinishInsertingNode();
}

void SVGScriptElement::childrenChanged(const ChildChange& change)
{
    SVGElement::childrenChanged(change);
    ScriptElement::childrenChanged(change);
}

void SVGScriptElement::finishParsingChildren()
{
    SVGElement::finishParsingChildren();
    ScriptElement::finishParsingChildren();
}

void SVGScriptElement::addSubresourceAttributeURLs(ListHashSet<URL>& urls) const
{
    SVGElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, protectedDocument()->completeURL(href()));
}
Ref<Element> SVGScriptElement::cloneElementWithoutAttributesAndChildren(Document& document, CustomElementRegistry*) const
{
    return adoptRef(*new SVGScriptElement(tagQName(), document, false, alreadyStarted()));
}

void SVGScriptElement::dispatchErrorEvent()
{
    setErrorOccurred(true);
    ScriptElement::dispatchErrorEvent();
}

}

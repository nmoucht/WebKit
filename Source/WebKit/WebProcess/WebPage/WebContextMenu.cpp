/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "WebContextMenu.h"

#if ENABLE(CONTEXT_MENUS)

#include "APIInjectedBundlePageContextMenuClient.h"
#include "ContextMenuContextData.h"
#include "MessageSenderInlines.h"
#include "UserData.h"
#include "WebFrame.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/ContextMenu.h>
#include <WebCore/ContextMenuController.h>
#include <WebCore/LocalFrame.h>
#include <WebCore/LocalFrameInlines.h>
#include <WebCore/LocalFrameView.h>
#include <WebCore/Page.h>

namespace WebKit {
using namespace WebCore;

WebContextMenu::WebContextMenu(WebPage& page)
    : m_page(page)
{
}

WebContextMenu::~WebContextMenu()
{
}

void WebContextMenu::show()
{
    Ref page = *m_page;
    auto& controller = page->corePage()->contextMenuController();
    RefPtr frame = controller.hitTestResult().innerNodeFrame();
    if (!frame)
        return;
    RefPtr webFrame = WebFrame::fromCoreFrame(*frame);
    if (!webFrame)
        return;
    RefPtr view = frame->view();
    if (!view)
        return;

    Vector<WebContextMenuItemData> menuItems;
    RefPtr<API::Object> userData;
    menuItemsWithUserData(menuItems, userData);

    auto menuLocation = view->contentsToRootView(controller.hitTestResult().roundedPointInInnerNodeFrame());

    ContextMenuContextData contextMenuContextData(menuLocation, menuItems, controller.context());

    page->showContextMenuFromFrame(webFrame->info(), contextMenuContextData, UserData(WebProcess::singleton().transformObjectsToHandles(userData.get()).get()));
}

void WebContextMenu::itemSelected(const WebContextMenuItemData& item)
{
    m_page->corePage()->contextMenuController().contextMenuItemSelected(static_cast<ContextMenuAction>(item.action()), item.title());
}

void WebContextMenu::menuItemsWithUserData(Vector<WebContextMenuItemData> &menuItems, RefPtr<API::Object>& userData) const
{
    ContextMenuController& controller = m_page->corePage()->contextMenuController();

    ContextMenu* menu = controller.contextMenu();
    if (!menu)
        return;

    // Give the bundle client a chance to process the menu.
    const Vector<ContextMenuItem>& coreItems = menu->items();

    RefPtr page = m_page.get();
    if (page->injectedBundleContextMenuClient().getCustomMenuFromDefaultItems(*page, controller.hitTestResult(), coreItems, menuItems, controller.context(), userData))
        return;
    menuItems = kitItems(coreItems);
}

Vector<WebContextMenuItemData> WebContextMenu::items() const
{
    Vector<WebContextMenuItemData> menuItems;
    RefPtr<API::Object> userData;
    menuItemsWithUserData(menuItems, userData);
    return menuItems;
}

} // namespace WebKit

#endif // ENABLE(CONTEXT_MENUS)

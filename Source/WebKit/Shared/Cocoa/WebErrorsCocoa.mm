/*
 * Copyright (C) 2010-2016 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebErrors.h"

#import "APIError.h"
#import "WKErrorRef.h"
#import <WebCore/LocalizedStrings.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/ResourceResponse.h>

namespace WebKit {
using namespace WebCore;

static RetainPtr<NSError> createNSError(NSString* domain, int code, NSURL *URL)
{
    RetainPtr<NSDictionary> userInfo = [NSDictionary dictionaryWithObjectsAndKeys:
        URL, NSURLErrorFailingURLErrorKey,
        [URL absoluteString], @"NSErrorFailingURLStringKey",
        nil];

    return adoptNS([[NSError alloc] initWithDomain:domain code:code userInfo:userInfo.get()]);
}

ResourceError cancelledError(const ResourceRequest& request)
{
    return ResourceError(createNSError(NSURLErrorDomain, NSURLErrorCancelled, request.url().createNSURL().get()).get());
}

ResourceError fileDoesNotExistError(const ResourceResponse& response)
{
    return ResourceError(createNSError(NSURLErrorDomain, NSURLErrorFileDoesNotExist, response.url().createNSURL().get()).get());
}

ResourceError decodeError(const URL& url)
{
    return ResourceError(createNSError(NSURLErrorDomain, NSURLErrorCannotDecodeContentData, url.createNSURL().get()).get());
}

} // namespace WebKit

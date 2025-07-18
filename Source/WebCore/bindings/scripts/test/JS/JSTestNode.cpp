/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "JSTestNode.h"

#include "ActiveDOMObject.h"
#include "ContextDestructionObserverInlines.h"
#include "DOMPromiseProxy.h"
#include "DeprecatedGlobalSettings.h"
#include "ExtendedDOMClientIsoSubspaces.h"
#include "ExtendedDOMIsoSubspaces.h"
#include "IDLTypes.h"
#include "JSDOMAttribute.h"
#include "JSDOMBinding.h"
#include "JSDOMConstructor.h"
#include "JSDOMConvertBase.h"
#include "JSDOMConvertBoolean.h"
#include "JSDOMConvertInterface.h"
#include "JSDOMConvertNumbers.h"
#include "JSDOMConvertPromise.h"
#include "JSDOMConvertStrings.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMGlobalObject.h"
#include "JSDOMGlobalObjectInlines.h"
#include "JSDOMIterator.h"
#include "JSDOMOperation.h"
#include "JSDOMOperationReturningPromise.h"
#include "JSDOMWrapperCache.h"
#include "JSTestNode.h"
#include "ScriptExecutionContext.h"
#include "WebCoreJSClientData.h"
#include <JavaScriptCore/BuiltinNames.h>
#include <JavaScriptCore/HeapAnalyzer.h>
#include <JavaScriptCore/JSCInlines.h>
#include <JavaScriptCore/JSDestructibleObjectHeapCellType.h>
#include <JavaScriptCore/ObjectConstructor.h>
#include <JavaScriptCore/SlotVisitorMacros.h>
#include <JavaScriptCore/SubspaceInlines.h>
#include <wtf/GetPtr.h>
#include <wtf/PointerPreparations.h>
#include <wtf/URL.h>
#include <wtf/text/MakeString.h>

namespace WebCore {
using namespace JSC;

// Functions

static JSC_DECLARE_HOST_FUNCTION(jsTestNodePrototypeFunction_testWorkerPromise);
static JSC_DECLARE_HOST_FUNCTION(jsTestNodePrototypeFunction_calculateSecretResult);
static JSC_DECLARE_HOST_FUNCTION(jsTestNodePrototypeFunction_getSecretBoolean);
#if ENABLE(TEST_FEATURE)
static JSC_DECLARE_HOST_FUNCTION(jsTestNodePrototypeFunction_testFeatureGetSecretBoolean);
#endif
static JSC_DECLARE_HOST_FUNCTION(jsTestNodePrototypeFunction_toJSON);
static JSC_DECLARE_HOST_FUNCTION(jsTestNodePrototypeFunction_entries);
static JSC_DECLARE_HOST_FUNCTION(jsTestNodePrototypeFunction_keys);
static JSC_DECLARE_HOST_FUNCTION(jsTestNodePrototypeFunction_values);
static JSC_DECLARE_HOST_FUNCTION(jsTestNodePrototypeFunction_forEach);

// Attributes

static JSC_DECLARE_CUSTOM_GETTER(jsTestNodeConstructor);
static JSC_DECLARE_CUSTOM_GETTER(jsTestNode_name);
static JSC_DECLARE_CUSTOM_SETTER(setJSTestNode_name);

class JSTestNodePrototype final : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestNodePrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestNodePrototype* ptr = new (NotNull, JSC::allocateCell<JSTestNodePrototype>(vm)) JSTestNodePrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    template<typename CellType, JSC::SubspaceAccess>
    static JSC::GCClient::IsoSubspace* subspaceFor(JSC::VM& vm)
    {
        STATIC_ASSERT_ISO_SUBSPACE_SHARABLE(JSTestNodePrototype, Base);
        return &vm.plainObjectSpace();
    }
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestNodePrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};
STATIC_ASSERT_ISO_SUBSPACE_SHARABLE(JSTestNodePrototype, JSTestNodePrototype::Base);

using JSTestNodeDOMConstructor = JSDOMConstructor<JSTestNode>;

template<> EncodedJSValue JSC_HOST_CALL_ATTRIBUTES JSTestNodeDOMConstructor::construct(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame)
{
    SUPPRESS_UNCOUNTED_LOCAL auto& vm = lexicalGlobalObject->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* castedThis = jsCast<JSTestNodeDOMConstructor*>(callFrame->jsCallee());
    ASSERT(castedThis);
    auto object = TestNode::create();
    if constexpr (IsExceptionOr<decltype(object)>)
        RETURN_IF_EXCEPTION(throwScope, { });
    static_assert(TypeOrExceptionOrUnderlyingType<decltype(object)>::isRef);
    auto jsValue = toJSNewlyCreated<IDLInterface<TestNode>>(*lexicalGlobalObject, *castedThis->globalObject(), throwScope, WTFMove(object));
    if constexpr (IsExceptionOr<decltype(object)>)
        RETURN_IF_EXCEPTION(throwScope, { });
    setSubclassStructureIfNeeded<TestNode>(lexicalGlobalObject, callFrame, asObject(jsValue));
    RETURN_IF_EXCEPTION(throwScope, { });
    return JSValue::encode(jsValue);
}
JSC_ANNOTATE_HOST_FUNCTION(JSTestNodeDOMConstructorConstruct, JSTestNodeDOMConstructor::construct);

template<> const ClassInfo JSTestNodeDOMConstructor::s_info = { "TestNode"_s, &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNodeDOMConstructor) };

template<> JSValue JSTestNodeDOMConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    return JSNode::getConstructor(vm, &globalObject);
}

template<> void JSTestNodeDOMConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    JSString* nameString = jsNontrivialString(vm, "TestNode"_s);
    m_originalName.set(vm, this, nameString);
    putDirect(vm, vm.propertyNames->name, nameString, JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->prototype, JSTestNode::prototype(vm, globalObject), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::DontDelete);
}

/* Hash table for prototype */

static const std::array<HashTableValue, 11> JSTestNodePrototypeTableValues {
    HashTableValue { "constructor"_s, static_cast<unsigned>(PropertyAttribute::DontEnum), NoIntrinsic, { HashTableValue::GetterSetterType, jsTestNodeConstructor, 0 } },
    HashTableValue { "name"_s, JSC::PropertyAttribute::CustomAccessor | JSC::PropertyAttribute::DOMAttribute, NoIntrinsic, { HashTableValue::GetterSetterType, jsTestNode_name, setJSTestNode_name } },
    HashTableValue { "testWorkerPromise"_s, static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { HashTableValue::NativeFunctionType, jsTestNodePrototypeFunction_testWorkerPromise, 0 } },
    HashTableValue { "calculateSecretResult"_s, static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { HashTableValue::NativeFunctionType, jsTestNodePrototypeFunction_calculateSecretResult, 0 } },
    HashTableValue { "getSecretBoolean"_s, static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { HashTableValue::NativeFunctionType, jsTestNodePrototypeFunction_getSecretBoolean, 0 } },
#if ENABLE(TEST_FEATURE)
    HashTableValue { "testFeatureGetSecretBoolean"_s, static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { HashTableValue::NativeFunctionType, jsTestNodePrototypeFunction_testFeatureGetSecretBoolean, 0 } },
#else
    HashTableValue { { }, 0, NoIntrinsic, { HashTableValue::End } },
#endif
    HashTableValue { "toJSON"_s, static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { HashTableValue::NativeFunctionType, jsTestNodePrototypeFunction_toJSON, 0 } },
    HashTableValue { "entries"_s, static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { HashTableValue::NativeFunctionType, jsTestNodePrototypeFunction_entries, 0 } },
    HashTableValue { "keys"_s, static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { HashTableValue::NativeFunctionType, jsTestNodePrototypeFunction_keys, 0 } },
    HashTableValue { "values"_s, static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { HashTableValue::NativeFunctionType, jsTestNodePrototypeFunction_values, 0 } },
    HashTableValue { "forEach"_s, static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { HashTableValue::NativeFunctionType, jsTestNodePrototypeFunction_forEach, 1 } },
};

const ClassInfo JSTestNodePrototype::s_info = { "TestNode"_s, &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNodePrototype) };

void JSTestNodePrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestNode::info(), JSTestNodePrototypeTableValues, *this);
    bool hasDisabledRuntimeProperties = false;
    if (!jsCast<JSDOMGlobalObject*>(globalObject())->scriptExecutionContext()->isSecureContext()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, "calculateSecretResult"_s);
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        DeletePropertySlot slot;
        JSObject::deleteProperty(this, globalObject(), propertyName, slot);
    }
    if (!jsCast<JSDOMGlobalObject*>(globalObject())->scriptExecutionContext()->isSecureContext()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, "getSecretBoolean"_s);
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        DeletePropertySlot slot;
        JSObject::deleteProperty(this, globalObject(), propertyName, slot);
    }
#if ENABLE(TEST_FEATURE)
    if (!(jsCast<JSDOMGlobalObject*>(globalObject())->scriptExecutionContext()->isSecureContext() && DeprecatedGlobalSettings::testFeatureEnabled())) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, "testFeatureGetSecretBoolean"_s);
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        DeletePropertySlot slot;
        JSObject::deleteProperty(this, globalObject(), propertyName, slot);
    }
#endif
    if (!DeprecatedGlobalSettings::domIteratorEnabled()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, "entries"_s);
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        DeletePropertySlot slot;
        JSObject::deleteProperty(this, globalObject(), propertyName, slot);
    }
    if (!DeprecatedGlobalSettings::domIteratorEnabled()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, "keys"_s);
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        DeletePropertySlot slot;
        JSObject::deleteProperty(this, globalObject(), propertyName, slot);
    }
    if (!DeprecatedGlobalSettings::domIteratorEnabled()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, "values"_s);
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        DeletePropertySlot slot;
        JSObject::deleteProperty(this, globalObject(), propertyName, slot);
    }
    if (!DeprecatedGlobalSettings::domIteratorEnabled()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, "forEach"_s);
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        DeletePropertySlot slot;
        JSObject::deleteProperty(this, globalObject(), propertyName, slot);
    }
    if (hasDisabledRuntimeProperties && structure()->isDictionary())
        flattenDictionaryObject(vm);
    putDirect(vm, vm.propertyNames->iteratorSymbol, getDirect(vm, vm.propertyNames->builtinNames().entriesPublicName()), static_cast<unsigned>(JSC::PropertyAttribute::DontEnum));
    JSC_TO_STRING_TAG_WITHOUT_TRANSITION();
}

const ClassInfo JSTestNode::s_info = { "TestNode"_s, &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNode) };

JSTestNode::JSTestNode(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestNode>&& impl)
    : JSNode(structure, globalObject, WTFMove(impl))
{
}

Ref<TestNode> JSTestNode::protectedWrapped() const
{
    return wrapped();
}

JSObject* JSTestNode::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    auto* structure = JSTestNodePrototype::createStructure(vm, &globalObject, JSNode::prototype(vm, globalObject));
    structure->setMayBePrototype(true);
    return JSTestNodePrototype::create(vm, &globalObject, structure);
}

JSObject* JSTestNode::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestNode>(vm, globalObject);
}

JSValue JSTestNode::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestNodeDOMConstructor, DOMConstructorID::TestNode>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

JSC_DEFINE_CUSTOM_GETTER(jsTestNodeConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    SUPPRESS_UNCOUNTED_LOCAL auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestNodePrototype*>(JSValue::decode(thisValue));
    if (!prototype) [[unlikely]]
        return throwVMTypeError(lexicalGlobalObject, throwScope);
    return JSValue::encode(JSTestNode::getConstructor(vm, prototype->globalObject()));
}

static inline JSValue jsTestNode_nameGetter(JSGlobalObject& lexicalGlobalObject, JSTestNode& thisObject)
{
    SUPPRESS_UNCOUNTED_LOCAL auto& vm = JSC::getVM(&lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    SUPPRESS_UNCOUNTED_LOCAL auto& impl = thisObject.wrapped();
    RELEASE_AND_RETURN(throwScope, (toJS<IDLDOMString>(lexicalGlobalObject, throwScope, impl.name())));
}

JSC_DEFINE_CUSTOM_GETTER(jsTestNode_name, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName attributeName))
{
    return IDLAttribute<JSTestNode>::get<jsTestNode_nameGetter, CastedThisErrorBehavior::Assert>(*lexicalGlobalObject, thisValue, attributeName);
}

static inline bool setJSTestNode_nameSetter(JSGlobalObject& lexicalGlobalObject, JSTestNode& thisObject, JSValue value)
{
    SUPPRESS_UNCOUNTED_LOCAL auto& vm = JSC::getVM(&lexicalGlobalObject);
    UNUSED_PARAM(vm);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    SUPPRESS_UNCOUNTED_LOCAL auto& impl = thisObject.wrapped();
    auto nativeValueConversionResult = convert<IDLDOMString>(lexicalGlobalObject, value);
    if (nativeValueConversionResult.hasException(throwScope)) [[unlikely]]
        return false;
    invokeFunctorPropagatingExceptionIfNecessary(lexicalGlobalObject, throwScope, [&] {
        return impl.setName(nativeValueConversionResult.releaseReturnValue());
    });
    return true;
}

JSC_DEFINE_CUSTOM_SETTER(setJSTestNode_name, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue, PropertyName attributeName))
{
    return IDLAttribute<JSTestNode>::set<setJSTestNode_nameSetter>(*lexicalGlobalObject, thisValue, encodedValue, attributeName);
}

static inline JSC::EncodedJSValue jsTestNodePrototypeFunction_testWorkerPromiseBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperationReturningPromise<JSTestNode>::ClassParameter castedThis, Ref<DeferredPromise>&& promise)
{
    SUPPRESS_UNCOUNTED_LOCAL auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    SUPPRESS_UNCOUNTED_LOCAL auto& impl = castedThis->wrapped();
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLPromise<IDLUndefined>>(*lexicalGlobalObject, *castedThis->globalObject(), throwScope, [&]() -> decltype(auto) { return impl.testWorkerPromise(WTFMove(promise)); })));
}

JSC_DEFINE_HOST_FUNCTION(jsTestNodePrototypeFunction_testWorkerPromise, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperationReturningPromise<JSTestNode>::call<jsTestNodePrototypeFunction_testWorkerPromiseBody>(*lexicalGlobalObject, *callFrame, "testWorkerPromise");
}

static inline JSC::EncodedJSValue jsTestNodePrototypeFunction_calculateSecretResultBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperationReturningPromise<JSTestNode>::ClassParameter castedThis, Ref<DeferredPromise>&& promise)
{
    SUPPRESS_UNCOUNTED_LOCAL auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    SUPPRESS_UNCOUNTED_LOCAL auto& impl = castedThis->wrapped();
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLPromise<IDLDouble>>(*lexicalGlobalObject, *castedThis->globalObject(), throwScope, [&]() -> decltype(auto) { return impl.calculateSecretResult(WTFMove(promise)); })));
}

JSC_DEFINE_HOST_FUNCTION(jsTestNodePrototypeFunction_calculateSecretResult, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperationReturningPromise<JSTestNode>::call<jsTestNodePrototypeFunction_calculateSecretResultBody>(*lexicalGlobalObject, *callFrame, "calculateSecretResult");
}

static inline JSC::EncodedJSValue jsTestNodePrototypeFunction_getSecretBooleanBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestNode>::ClassParameter castedThis)
{
    SUPPRESS_UNCOUNTED_LOCAL auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    SUPPRESS_UNCOUNTED_LOCAL auto& impl = castedThis->wrapped();
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLBoolean>(*lexicalGlobalObject, throwScope, impl.getSecretBoolean())));
}

JSC_DEFINE_HOST_FUNCTION(jsTestNodePrototypeFunction_getSecretBoolean, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunction_getSecretBooleanBody>(*lexicalGlobalObject, *callFrame, "getSecretBoolean");
}

#if ENABLE(TEST_FEATURE)
static inline JSC::EncodedJSValue jsTestNodePrototypeFunction_testFeatureGetSecretBooleanBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestNode>::ClassParameter castedThis)
{
    SUPPRESS_UNCOUNTED_LOCAL auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    SUPPRESS_UNCOUNTED_LOCAL auto& impl = castedThis->wrapped();
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLBoolean>(*lexicalGlobalObject, throwScope, impl.testFeatureGetSecretBoolean())));
}

JSC_DEFINE_HOST_FUNCTION(jsTestNodePrototypeFunction_testFeatureGetSecretBoolean, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunction_testFeatureGetSecretBooleanBody>(*lexicalGlobalObject, *callFrame, "testFeatureGetSecretBoolean");
}

#endif

static inline EncodedJSValue jsTestNodePrototypeFunction_toJSONBody(JSGlobalObject* lexicalGlobalObject, CallFrame*, JSTestNode* castedThis)
{
    SUPPRESS_UNCOUNTED_LOCAL auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    SUPPRESS_UNCOUNTED_LOCAL auto& impl = castedThis->wrapped();
    auto* result = constructEmptyObject(lexicalGlobalObject);
    auto nameValue = toJS<IDLDOMString>(*lexicalGlobalObject, throwScope, impl.name());
    RETURN_IF_EXCEPTION(throwScope, { });
    result->putDirect(vm, Identifier::fromString(vm, "name"_s), nameValue);
    return JSValue::encode(result);
}

JSC_DEFINE_HOST_FUNCTION(jsTestNodePrototypeFunction_toJSON, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunction_toJSONBody>(*lexicalGlobalObject, *callFrame, "toJSON");
}

struct TestNodeIteratorTraits {
    static constexpr JSDOMIteratorType type = JSDOMIteratorType::Set;
    using KeyType = void;
    using ValueType = IDLInterface<TestNode>;
};

using TestNodeIteratorBase = JSDOMIteratorBase<JSTestNode, TestNodeIteratorTraits>;
class TestNodeIterator final : public TestNodeIteratorBase {
public:
    using Base = TestNodeIteratorBase;
    DECLARE_INFO;

    template<typename, SubspaceAccess mode> static JSC::GCClient::IsoSubspace* subspaceFor(JSC::VM& vm)
    {
        if constexpr (mode == JSC::SubspaceAccess::Concurrently)
            return nullptr;
        return WebCore::subspaceForImpl<TestNodeIterator, UseCustomHeapCellType::No>(vm, "TestNodeIterator"_s,
            [] (auto& spaces) { return spaces.m_clientSubspaceForTestNodeIterator.get(); },
            [] (auto& spaces, auto&& space) { spaces.m_clientSubspaceForTestNodeIterator = std::forward<decltype(space)>(space); },
            [] (auto& spaces) { return spaces.m_subspaceForTestNodeIterator.get(); },
            [] (auto& spaces, auto&& space) { spaces.m_subspaceForTestNodeIterator = std::forward<decltype(space)>(space); }
        );
    }

    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

    static TestNodeIterator* create(JSC::VM& vm, JSC::Structure* structure, JSTestNode& iteratedObject, IterationKind kind)
    {
        auto* instance = new (NotNull, JSC::allocateCell<TestNodeIterator>(vm)) TestNodeIterator(structure, iteratedObject, kind);
        instance->finishCreation(vm);
        return instance;
    }
private:
    TestNodeIterator(JSC::Structure* structure, JSTestNode& iteratedObject, IterationKind kind)
        : Base(structure, iteratedObject, kind)
    {
    }
};

using TestNodeIteratorPrototype = JSDOMIteratorPrototype<JSTestNode, TestNodeIteratorTraits>;
JSC_ANNOTATE_HOST_FUNCTION(TestNodeIteratorPrototypeNext, TestNodeIteratorPrototype::next);

template<>
const JSC::ClassInfo TestNodeIteratorBase::s_info = { "TestNodeBase Iterator"_s, &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(TestNodeIteratorBase) };
const JSC::ClassInfo TestNodeIterator::s_info = { "TestNode Iterator"_s, &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(TestNodeIterator) };

template<>
const JSC::ClassInfo TestNodeIteratorPrototype::s_info = { "TestNode Iterator"_s, &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(TestNodeIteratorPrototype) };

static inline EncodedJSValue jsTestNodePrototypeFunction_entriesCaller(JSGlobalObject*, CallFrame*, JSTestNode* thisObject)
{
    return JSValue::encode(iteratorCreate<TestNodeIterator>(*thisObject, IterationKind::Values));
}

JSC_DEFINE_HOST_FUNCTION(jsTestNodePrototypeFunction_entries, (JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame))
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunction_entriesCaller>(*lexicalGlobalObject, *callFrame, "entries");
}

static inline EncodedJSValue jsTestNodePrototypeFunction_keysCaller(JSGlobalObject*, CallFrame*, JSTestNode* thisObject)
{
    return JSValue::encode(iteratorCreate<TestNodeIterator>(*thisObject, IterationKind::Keys));
}

JSC_DEFINE_HOST_FUNCTION(jsTestNodePrototypeFunction_keys, (JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame))
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunction_keysCaller>(*lexicalGlobalObject, *callFrame, "keys");
}

static inline EncodedJSValue jsTestNodePrototypeFunction_valuesCaller(JSGlobalObject*, CallFrame*, JSTestNode* thisObject)
{
    return JSValue::encode(iteratorCreate<TestNodeIterator>(*thisObject, IterationKind::Values));
}

JSC_DEFINE_HOST_FUNCTION(jsTestNodePrototypeFunction_values, (JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame))
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunction_valuesCaller>(*lexicalGlobalObject, *callFrame, "values");
}

static inline EncodedJSValue jsTestNodePrototypeFunction_forEachCaller(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame, JSTestNode* thisObject)
{
    return JSValue::encode(iteratorForEach<TestNodeIterator>(*lexicalGlobalObject, *callFrame, *thisObject));
}

JSC_DEFINE_HOST_FUNCTION(jsTestNodePrototypeFunction_forEach, (JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame))
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunction_forEachCaller>(*lexicalGlobalObject, *callFrame, "forEach");
}

JSC::GCClient::IsoSubspace* JSTestNode::subspaceForImpl(JSC::VM& vm)
{
    return WebCore::subspaceForImpl<JSTestNode, UseCustomHeapCellType::No>(vm, "JSTestNode"_s,
        [] (auto& spaces) { return spaces.m_clientSubspaceForTestNode.get(); },
        [] (auto& spaces, auto&& space) { spaces.m_clientSubspaceForTestNode = std::forward<decltype(space)>(space); },
        [] (auto& spaces) { return spaces.m_subspaceForTestNode.get(); },
        [] (auto& spaces, auto&& space) { spaces.m_subspaceForTestNode = std::forward<decltype(space)>(space); }
    );
}

void JSTestNode::analyzeHeap(JSCell* cell, HeapAnalyzer& analyzer)
{
    auto* thisObject = jsCast<JSTestNode*>(cell);
    analyzer.setWrappedObjectForCell(cell, &thisObject->wrapped());
    if (RefPtr context = thisObject->scriptExecutionContext())
        analyzer.setLabelForCell(cell, makeString("url "_s, context->url().string()));
    Base::analyzeHeap(cell, analyzer);
}

WTF_ALLOW_UNSAFE_BUFFER_USAGE_BEGIN
#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestNode@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore8TestNodeE[]; }
#endif
template<std::same_as<TestNode> T>
static inline void verifyVTable(TestNode* ptr) 
{
    if constexpr (std::is_polymorphic_v<T>) {
        const void* actualVTablePointer = getVTablePointer<T>(ptr);
#if PLATFORM(WIN)
        void* expectedVTablePointer = __identifier("??_7TestNode@WebCore@@6B@");
#else
        void* expectedVTablePointer = &_ZTVN7WebCore8TestNodeE[2];
#endif

        // If you hit this assertion you either have a use after free bug, or
        // TestNode has subclasses. If TestNode has subclasses that get passed
        // to toJS() we currently require TestNode you to opt out of binding hardening
        // by adding the SkipVTableValidation attribute to the interface IDL definition
        RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
    }
}
#endif
WTF_ALLOW_UNSAFE_BUFFER_USAGE_END

JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject*, JSDOMGlobalObject* globalObject, Ref<TestNode>&& impl)
{
#if ENABLE(BINDING_INTEGRITY)
    verifyVTable<TestNode>(impl.ptr());
#endif
    return createWrapper<TestNode>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, TestNode& impl)
{
    return wrap(lexicalGlobalObject, globalObject, impl);
}


}

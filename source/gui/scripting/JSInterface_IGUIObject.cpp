/* Copyright (C) 2019 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "JSInterface_IGUIObject.h"

#include "gui/CGUI.h"
#include "gui/CGUIColor.h"
#include "gui/CList.h"
#include "gui/GUIManager.h"
#include "gui/IGUIObject.h"
#include "gui/IGUIScrollBar.h"
#include "gui/scripting/JSInterface_GUITypes.h"
#include "ps/CLogger.h"
#include "scriptinterface/ScriptExtraHeaders.h"
#include "scriptinterface/ScriptInterface.h"

JSClass JSI_IGUIObject::JSI_class = {
	"GUIObject", JSCLASS_HAS_PRIVATE,
	nullptr, nullptr,
	JSI_IGUIObject::getProperty, JSI_IGUIObject::setProperty,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr
};

JSFunctionSpec JSI_IGUIObject::JSI_methods[] =
{
	JS_FN("toString", JSI_IGUIObject::toString, 0, 0),
	JS_FN("focus", JSI_IGUIObject::focus, 0, 0),
	JS_FN("blur", JSI_IGUIObject::blur, 0, 0),
	JS_FN("getComputedSize", JSI_IGUIObject::getComputedSize, 0, 0),
	JS_FS_END
};

bool JSI_IGUIObject::getProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp)
{
	JSAutoRequest rq(cx);
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;

	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, obj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	JS::RootedValue idval(cx);
	if (!JS_IdToValue(cx, id, &idval))
		return false;

	std::string propName;
	if (!ScriptInterface::FromJSVal(cx, idval, propName))
		return false;

	// Skip registered functions and inherited properties
	// including JSInterfaces of derived classes
	if (propName == "constructor" ||
		propName == "prototype"   ||
		propName == "toString"    ||
		propName == "toJSON"      ||
		propName == "focus"       ||
		propName == "blur"        ||
		propName == "getTextSize" ||
		propName == "getComputedSize"
	   )
		return true;

	// Use onWhatever to access event handlers
	if (propName.substr(0, 2) == "on")
	{
		CStr eventName(CStr(propName.substr(2)).LowerCase());
		std::map<CStr, JS::Heap<JSObject*>>::iterator it = e->m_ScriptHandlers.find(eventName);
		if (it == e->m_ScriptHandlers.end())
			vp.setNull();
		else
			vp.setObject(*it->second.get());
		return true;
	}

	if (propName == "parent")
	{
		IGUIObject* parent = e->GetParent();

		if (parent)
			vp.set(JS::ObjectValue(*parent->GetJSObject()));
		else
			vp.set(JS::NullValue());

		return true;
	}
	else if (propName == "children")
	{
		pScriptInterface->CreateArray(vp);

		for (size_t i = 0; i < e->m_Children.size(); ++i)
			pScriptInterface->SetPropertyInt(vp, i, e->m_Children[i]);

		return true;
	}
	else if (propName == "name")
	{
		ScriptInterface::ToJSVal(cx, vp, e->GetName());
		return true;
	}
	else if (e->SettingExists(propName))
	{
		e->m_Settings[propName]->ToJSVal(cx, vp);
		return true;
	}

	JS_ReportError(cx, "Property '%s' does not exist!", propName.c_str());
	return false;
}

bool JSI_IGUIObject::setProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp, JS::ObjectOpResult& result)
{
	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, obj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return result.fail(JSMSG_NOT_NONNULL_OBJECT);

	JSAutoRequest rq(cx);
	JS::RootedValue idval(cx);
	if (!JS_IdToValue(cx, id, &idval))
		return result.fail(JSMSG_NOT_NONNULL_OBJECT);

	std::string propName;
	if (!ScriptInterface::FromJSVal(cx, idval, propName))
		return result.fail(JSMSG_UNDEFINED_PROP);

	if (propName == "name")
	{
		std::string value;
		if (!ScriptInterface::FromJSVal(cx, vp, value))
			return result.fail(JSMSG_UNDEFINED_PROP);
		e->SetName(value);
		return result.succeed();
	}

	JS::RootedObject vpObj(cx);
	if (vp.isObject())
		vpObj = &vp.toObject();

	// Use onWhatever to set event handlers
	if (propName.substr(0, 2) == "on")
	{
		if (vp.isPrimitive() || vp.isNull() || !JS_ObjectIsFunction(cx, &vp.toObject()))
		{
			JS_ReportError(cx, "on- event-handlers must be functions");
			return result.fail(JSMSG_NOT_FUNCTION);
		}

		CStr eventName(CStr(propName.substr(2)).LowerCase());
		e->SetScriptHandler(eventName, vpObj);

		return result.succeed();
	}

	if (e->SettingExists(propName))
		return e->m_Settings[propName]->FromJSVal(cx, vp) ? result.succeed() : result.fail(JSMSG_TYPE_ERR_BAD_ARGS);

	JS_ReportError(cx, "Property '%s' does not exist!", propName.c_str());
	return result.fail(JSMSG_UNDEFINED_PROP);
}

void JSI_IGUIObject::init(ScriptInterface& scriptInterface)
{
	scriptInterface.DefineCustomObjectType(&JSI_class, nullptr, 1, nullptr, JSI_methods, nullptr, nullptr);
}

bool JSI_IGUIObject::toString(JSContext* cx, uint UNUSED(argc), JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	JS::RootedObject thisObj(cx, JS_THIS_OBJECT(cx, vp));

	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, thisObj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	ScriptInterface::ToJSVal(cx, rec.rval(), "[GUIObject: " + e->GetName() + "]");
	return true;
}

bool JSI_IGUIObject::focus(JSContext* cx, uint UNUSED(argc), JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	JS::RootedObject thisObj(cx, JS_THIS_OBJECT(cx, vp));

	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, thisObj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	e->GetGUI()->SetFocusedObject(e);

	rec.rval().setUndefined();
	return true;
}

bool JSI_IGUIObject::blur(JSContext* cx, uint UNUSED(argc), JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	JS::RootedObject thisObj(cx, JS_THIS_OBJECT(cx, vp));

	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, thisObj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	e->GetGUI()->SetFocusedObject(NULL);

	rec.rval().setUndefined();
	return true;
}

bool JSI_IGUIObject::getComputedSize(JSContext* cx, uint UNUSED(argc), JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	JS::RootedObject thisObj(cx, JS_THIS_OBJECT(cx, vp));

	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, thisObj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	e->UpdateCachedSize();
	CRect size = e->m_CachedActualSize;

	JS::RootedValue objVal(cx);
	try
	{
		ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface->CreateObject(
			&objVal,
			"left", size.left,
			"right", size.right,
			"top", size.top,
			"bottom", size.bottom);
	}
	catch (PSERROR_Scripting_ConversionFailed&)
	{
		debug_warn(L"Error creating size object!");
		return false;
	}

	rec.rval().set(objVal);
	return true;
}

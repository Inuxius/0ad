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

/*
GUI util

--Overview--

	Contains help class GUI<>, which gives us templated
	 parameter to all functions within GUI.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_GUIUTIL
#define INCLUDED_GUIUTIL

#include "gui/CGUI.h"
#include "gui/CGUISprite.h"
#include "gui/GUIbase.h"
#include "gui/IGUIObject.h"

#include <functional>

class CClientArea;
class CGUIString;
template<typename T> class GUI;

class IGUISetting
{
public:
	NONCOPYABLE(IGUISetting);

	IGUISetting() = default;
	virtual ~IGUISetting() = default;

	/**
	 * Parses the given string and assigns to the setting value. Used for parsing XML attributes.
	 */
	virtual bool FromString(const CStrW& Value, const bool& SkipMessage) = 0;

	/**
	 * Parses the given JS::Value using ScriptInterface::FromJSVal and assigns it to the setting data.
	 */
	virtual bool FromJSVal(JSContext* cx, JS::HandleValue Value) = 0;

	/**
	 * Converts the setting data to a JS::Value using ScriptInterface::ToJSVal.
	 */
	virtual void ToJSVal(JSContext* cx, JS::MutableHandleValue Value) = 0;
};

template<typename T>
class CGUISetting : public IGUISetting
{
	friend class GUI<T>;

public:
	NONCOPYABLE(CGUISetting);

	CGUISetting(IGUIObject& pObject, const CStr& Name);

	/**
	 * Parses the given string and assigns to the setting value. Used for parsing XML attributes.
	 */
	bool FromString(const CStrW& Value, const bool& SkipMessage) override;

	/**
	 * Parses the given JS::Value using ScriptInterface::FromJSVal and assigns it to the setting data.
	 */
	bool FromJSVal(JSContext* cx, JS::HandleValue Value) override;

	/**
	 * Converts the setting data to a JS::Value using ScriptInterface::ToJSVal.
	 */
	void ToJSVal(JSContext* cx, JS::MutableHandleValue Value) override;

private:

	/**
	 * The object that stores this setting.
	 */
	IGUIObject& m_pObject;

	/**
	 * Property name identifying the setting.
	 */
	const CStr m_Name;

	/**
	 * Holds the value of the setting..
	 */
	T m_pSetting;
};

template <typename T>
bool __ParseString(const CStrW& Value, T& tOutput);

struct SGUIMessage;

/**
 * Includes static functions that needs one template
 * argument.
 *
 * int is only to please functions that doesn't even use T
 * and are only within this class because it's convenient
 */
template <typename T=int>
class GUI
{
	// Private functions further ahead
	friend class CGUI;
	friend class IGUIObject;

public:
	NONCOPYABLE(GUI);

	// Like GetSetting (below), but doesn't make a copy of the value
	// (so it can be modified later)
	static PSRETURN GetSettingPointer(const IGUIObject* pObject, const CStr& Setting, T*& Value);

	/**
	 * Retrieves a setting by name from object pointer
	 *
	 * @param pObject Object pointer
	 * @param Setting Setting by name
	 * @param Value Stores value here, note type T!
	 */
	static PSRETURN GetSetting(const IGUIObject* pObject, const CStr& Setting, T& Value);

	/**
	 * Sets a value by name using a real datatype as input.
	 *
	 * This is the official way of setting a setting, no other
	 *  way should only cautiously be used!
	 *
	 * This variant will use the move-assignment.
	 *
	 * @param pObject Object pointer
	 * @param Setting Setting by name
	 * @param Value Sets value to this, note type T!
	 * @param SkipMessage Does not send a GUIM_SETTINGS_UPDATED if true
	 */
	static PSRETURN SetSetting(IGUIObject* pObject, const CStr& Setting, T& Value, const bool& SkipMessage = false);

	/**
	 * This variant will copy the value.
	 */
	static PSRETURN SetSetting(IGUIObject* pObject, const CStr& Setting, const T& Value, const bool& SkipMessage = false);

	/**
	 * This will return the value of the first sprite if it's not null,
	 * if it is null, it will return the value of the second sprite, if
	 * that one is null, then null it is.
	 *
	 * @param prim Primary sprite that should be used
	 * @param sec Secondary sprite if Primary should fail
	 * @return Resulting string
	 */
	static const CGUISpriteInstance& FallBackSprite(const CGUISpriteInstance& prim, const CGUISpriteInstance& sec)
	{
		return (prim.IsEmpty() ? sec : prim);
	}

	/**
	 * Same principle as FallBackSprite
	 *
	 * @param prim Primary color that should be used
	 * @param sec Secondary color if Primary should fail
	 * @return Resulting color
	 * @see FallBackSprite
	 */
	static CGUIColor FallBackColor(const CGUIColor& prim, const CGUIColor& sec)
	{
		// CGUIColor() == null.
		return ((prim!=CGUIColor())?(prim):(sec));
	}

	/**
	 * Sets a value by setting and object name using a real
	 * datatype as input.
	 *
	 * This is just a wrapper for __ParseString() which really
	 * works the magic.
	 *
	 * @param Value The value in string form, like "0 0 100% 100%"
	 * @param tOutput Parsed value of type T
	 * @return True at success.
	 *
	 * @see __ParseString()
	 */
	static bool ParseString(const CStrW& Value, T& tOutput)
	{
		return __ParseString<T>(Value, tOutput);
	}

	static bool ParseColor(const CStrW& Value, CGUIColor& tOutput, int DefaultAlpha);

private:

	/**
	 * Changes the value of the setting by calling the valueSet functon that performs either a copy or move assignment.
	 * Updates some internal data depending on the setting changed.
	 */
	static PSRETURN SetSettingWrap(IGUIObject* pObject, const CStr& Setting, const T& Value, const bool& SkipMessage, const std::function<void()>& valueSet);

	// templated typedef of function pointer
	typedef void (IGUIObject::*void_Object_pFunction_argT)(const T& arg);
	typedef void (IGUIObject::*void_Object_pFunction_argRefT)(T& arg);
	typedef void (IGUIObject::*void_Object_pFunction)();
	typedef void (IGUIObject::*void_Object_pFunction_argTJS)(const T& arg, JS::HandleValueArray paramData);

	/**
	 * If you want to call a IGUIObject-function
	 * on not just an object, but also on ALL of their children
	 * you want to use this recursion system.
	 * It recurses an object calling a function on itself
	 * and all children (and so forth).
	 *
	 * <b>Restrictions:</b>\n
	 * You can also set restrictions, so that if the recursion
	 * reaches an objects with certain setup, it just doesn't
	 * call the function on the object, nor it's children for
	 * that matter. i.e. it cuts that object off from the
	 * recursion tree. What setups that can cause restrictions
	 * are hardcoded and specific. Check out the defines
	 * GUIRR_* for all different setups.
	 *
	 * Error reports are either logged or thrown out of RecurseObject.
	 * Always use it with try/catch!
	 *
	 * @param RR Recurse Restrictions, set to 0 if no restrictions
	 * @param pObject Top object, this is where the iteration starts
	 * @param pFunc Function to recurse
	 * @param Argument Argument for pFunc of type T
	 * @throws PSERROR Depends on what pFunc might throw. PSERROR is standard.
	 *			Itself doesn't throw anything.
	*/
	static void RecurseObject(int RR, IGUIObject* pObject, void_Object_pFunction_argT pFunc, const T& Argument)
	{
		// TODO Gee: Don't run this for the base object.
		if (CheckIfRestricted(RR, pObject))
			return;

		(pObject->*pFunc)(Argument);

		// Iterate children
		for (IGUIObject* const& obj : *pObject)
			RecurseObject(RR, obj, pFunc, Argument);
	}

	/**
	 * Argument is reference.
	 *
	 * @see RecurseObject()
	 */
	static void RecurseObject(int RR, IGUIObject* pObject, void_Object_pFunction_argRefT pFunc, T& Argument)
	{
		if (CheckIfRestricted(RR, pObject))
			return;

		(pObject->*pFunc)(Argument);

		// Iterate children
		for (IGUIObject* const& obj : *pObject)
			RecurseObject(RR, obj, pFunc, Argument);
	}

	static void RecurseObject(int RR, IGUIObject* pObject, void_Object_pFunction_argTJS pFunc, const T& Argument, JS::HandleValueArray paramData)
	{
		if (CheckIfRestricted(RR, pObject))
			return;

		(pObject->*pFunc)(Argument, paramData);

		// Iterate children
		for (IGUIObject* const& obj : *pObject)
			RecurseObject(RR, obj, pFunc, Argument, paramData);
	}

	/**
	 * With no argument.
	 *
	 * @see RecurseObject()
	 */
	static void RecurseObject(int RR, IGUIObject* pObject, void_Object_pFunction pFunc)
	{
		if (CheckIfRestricted(RR, pObject))
			return;

		(pObject->*pFunc)();

		// Iterate children
		for (IGUIObject* const& obj : *pObject)
			RecurseObject(RR, obj, pFunc);
	}

	/**
	 * Checks restrictions for the iteration, for instance if
	 * you tell the recursor to avoid all hidden objects, it
	 * will, and this function checks a certain object's
	 * restriction values.
	 *
	 * @param RR What kind of restriction, for instance hidden or disabled
	 * @param pObject Object
	 * @return true if restricted
	 */
	static bool CheckIfRestricted(int RR, IGUIObject* pObject)
	{
		// Statically initialise some strings, so we don't have to do
		// lots of allocation every time this function is called
		static const CStr strHidden("hidden");
		static const CStr strEnabled("enabled");
		static const CStr strGhost("ghost");

		if (RR & GUIRR_HIDDEN)
		{
			bool hidden = true;
			GUI<bool>::GetSetting(pObject, strHidden, hidden);

			if (hidden)
				return true;
		}
		if (RR & GUIRR_DISABLED)
		{
			bool enabled = false;
			GUI<bool>::GetSetting(pObject, strEnabled, enabled);

			if (!enabled)
				return true;
		}
		if (RR & GUIRR_GHOST)
		{
			bool ghost = true;
			GUI<bool>::GetSetting(pObject, strGhost, ghost);

			if (ghost)
				return true;
		}

		// false means not restricted
		return false;
	}
};

#endif // INCLUDED_GUIUTIL

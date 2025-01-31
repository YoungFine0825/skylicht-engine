/*
!@
MIT License

Copyright (c) 2023 Skylicht Technology CO., LTD

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files
(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

This file is part of the "Skylicht Engine".
https://github.com/skylicht-lab/skylicht-engine
!#
*/

#pragma once

#include "CUIEventManager.h"
#include "Components/CComponentSystem.h"

#include "CUIBase.h"

namespace Skylicht
{
	namespace UI
	{
		class CUIContainer : public CComponentSystem
		{
		protected:
			bool m_enable;

			std::vector<CUIBase*> m_arrayUIObjects;
			std::vector<CUIBase*> m_raycastUIObjects;

			CUIBase* m_hover;
			CCanvas* m_canvas;

			bool m_inMotion;
			bool m_outMotion;

			bool m_pointerDown;

		public:

			std::function<void()> OnMotionInFinish;
			std::function<void()> OnMotionOutFinish;

		public:
			CUIContainer();

			virtual ~CUIContainer();

			virtual void initComponent();

			virtual void updateComponent();

			CCanvas* getCanvas();

			void addChild(CUIBase* base);

			bool removeChild(CUIBase* base);

			void removeChildsByGUI(CGUIElement* element);

			CUIBase* getChildByGUI(CGUIElement* element);

			virtual CUIBase* OnProcessEvent(const SEvent& event);

			virtual CUIBase* OnProcessEvent(const SEvent& event, CUIBase* capture);

			void onPointerOut(float x, float y);

			void startInMotion();

			void startOutMotion();

			inline void setEnable(bool b)
			{
				m_enable = b;
			}

			inline bool isEnable()
			{
				return m_enable;
			}
		};
	}
}
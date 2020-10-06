/*
!@
MIT License

Copyright (c) 2020 Skylicht Technology CO., LTD

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

#include "pch.h"
#include "CPostProcessorRP.h"
#include "Material/Shader/CShaderManager.h"
#include "Material/Shader/CShaderParams.h"
#include "Material/Shader/ShaderCallback/CShaderMaterial.h"

namespace Skylicht
{
	CPostProcessorRP::CPostProcessorRP() :
		m_adaptLum(NULL),
		m_lumTarget(0),
		m_bloomEffect(false),
		m_fxaa(false),
		m_brightFilter(NULL),
		m_blurFilter(NULL),
		m_bloomFilter(NULL),
		m_fxaaFilter(NULL)
	{
		m_luminance[0] = NULL;
		m_luminance[1] = NULL;

		for (int i = 0; i < 4; i++)
			m_rtt[i] = NULL;
	}

	CPostProcessorRP::~CPostProcessorRP()
	{
		IVideoDriver *driver = getVideoDriver();
		driver->removeTexture(m_luminance[0]);
		driver->removeTexture(m_luminance[1]);
		driver->removeTexture(m_adaptLum);

		for (int i = 0; i < 4; i++)
		{
			if (m_rtt[i] != NULL)
				driver->removeTexture(m_rtt[i]);
		}

		if (m_brightFilter != NULL)
			delete m_brightFilter;

		if (m_blurFilter != NULL)
			delete m_blurFilter;

		if (m_bloomFilter != NULL)
			delete m_bloomFilter;

		if (m_fxaaFilter != NULL)
			delete m_fxaaFilter;
	}

	void CPostProcessorRP::initRender(int w, int h)
	{
		IVideoDriver *driver = getVideoDriver();
		CShaderManager *shaderMgr = CShaderManager::getInstance();

		// init size of framebuffer
		m_size = core::dimension2du((u32)w, (u32)h);
		m_lumSize = core::dimension2du(1024, 1024);

		m_luminance[0] = driver->addRenderTargetTexture(m_lumSize, "lum_0", ECF_R16F);
		m_luminance[1] = driver->addRenderTargetTexture(m_lumSize, "lum_1", ECF_R16F);
		m_adaptLum = driver->addRenderTargetTexture(m_lumSize, "lum_adapt", ECF_R16F);

		// framebuffer for glow/fxaa
		if (m_bloomEffect == true || m_fxaa == true)
		{
			m_rtt[0] = driver->addRenderTargetTexture(m_size, "rtt_0", ECF_A16B16G16R16F);
			m_rtt[1] = driver->addRenderTargetTexture(m_size, "rtt_1", ECF_A16B16G16R16F);

			if (m_bloomEffect == true)
			{
				m_rtt[2] = driver->addRenderTargetTexture(m_size / 2, "rtt_2", ECF_A16B16G16R16F);
				m_rtt[3] = driver->addRenderTargetTexture(m_size / 4, "rtt_3", ECF_A16B16G16R16F);
			}
		}

		// init final pass shader
		m_finalPass.MaterialType = shaderMgr->getShaderIDByName("PostEffect");

		// init lum pass shader
		m_lumPass.MaterialType = shaderMgr->getShaderIDByName("Luminance");
		m_adaptLumPass.MaterialType = shaderMgr->getShaderIDByName("AdaptLuminance");

		m_brightFilter = new CMaterial("BrightFilter", "BuiltIn/Shader/PostProcessing/BrightFilter.xml");
		m_blurFilter = new CMaterial("DownBlurFilter", "BuiltIn/Shader/PostProcessing/DownsampleFilter.xml");
		m_bloomFilter = new CMaterial("BloomFilter", "BuiltIn/Shader/PostProcessing/Bloom.xml");
		m_fxaaFilter = new CMaterial("BloomFilter", "BuiltIn/Shader/PostProcessing/FXAA.xml");
	}

	void CPostProcessorRP::render(ITexture *target, CCamera *camera, CEntityManager *entityManager, const core::recti& viewport)
	{
		if (camera == NULL)
			return;

		onNext(target, camera, entityManager, viewport);
	}

	void CPostProcessorRP::luminanceMapGeneration(ITexture *color)
	{
		float w = (float)m_lumSize.Width;
		float h = (float)m_lumSize.Height;

		// Step 1: Generate target luminance from color to lum[0]
		IVideoDriver * driver = getVideoDriver();
		driver->setRenderTarget(m_adaptLum, true, true);

		m_lumPass.setTexture(0, color);

		beginRender2D(w, h);
		renderBufferToTarget(0.0f, 0.0f, w, h, m_lumPass);

		// Step 2: Interpolate to target luminance
		driver->setRenderTarget(m_luminance[m_lumTarget], true, true);

		m_adaptLumPass.setTexture(0, m_adaptLum);
		m_adaptLumPass.setTexture(1, m_luminance[!m_lumTarget]);

		beginRender2D(w, h);
		renderBufferToTarget(0.0f, 0.0f, w, h, m_adaptLumPass);
	}

	void CPostProcessorRP::brightFilter(int from, int to)
	{
		video::SVec4 curve;

		float threshold = 0.9f;
		float softKnee = 0.5f;
		curve.W = threshold;

		float knee = threshold * softKnee + 1e-5f;
		curve.X = threshold - knee;
		curve.Y = knee * 2.0f;
		curve.Z = 0.25f / knee;

		m_brightFilter->setUniform4("uCurve", &curve.X);
		renderEffect(from, to, m_brightFilter);
	}

	void CPostProcessorRP::blurDown(int from, int to)
	{
		core::dimension2du rrtSize = m_rtt[from]->getSize();
		core::vector2df blurSize;
		blurSize.X = 1.0f / (float)rrtSize.Width;
		blurSize.Y = 1.0f / (float)rrtSize.Height;
		m_blurFilter->setUniform2("uTexelSize", &blurSize.X);
		renderEffect(from, to, m_blurFilter);
	}

	void CPostProcessorRP::blurUp(int from, int to)
	{
		core::dimension2du rrtSize = m_rtt[from]->getSize();
		core::vector2df blurSize;
		blurSize.X = 1.0f / (float)rrtSize.Width;
		blurSize.Y = 1.0f / (float)rrtSize.Height;
		m_blurFilter->setUniform2("uTexelSize", &blurSize.X);
		renderEffect(from, to, m_blurFilter);
	}

	void CPostProcessorRP::postProcessing(ITexture *finalTarget, ITexture *color, ITexture *normal, ITexture *position, const core::recti& viewport)
	{
		IVideoDriver *driver = getVideoDriver();

		float renderW = (float)m_size.Width;
		float renderH = (float)m_size.Height;

		if (viewport.getWidth() > 0 && viewport.getHeight() > 0)
		{
			driver->setViewPort(viewport);
			renderW = (float)viewport.getWidth();
			renderH = (float)viewport.getHeight();
		}

		luminanceMapGeneration(color);

		if (m_bloomEffect || m_fxaa)
		{
			driver->setRenderTarget(m_rtt[0]);
		}
		else
		{
			driver->setRenderTarget(finalTarget);
		}

		m_luminance[m_lumTarget]->regenerateMipMapLevels();

		m_finalPass.setTexture(0, color);
		m_finalPass.setTexture(1, m_luminance[m_lumTarget]);

		beginRender2D(renderW, renderH);
		renderBufferToTarget(0.0f, 0.0f, renderW, renderH, m_finalPass);

		int colorID = 0;

		if (m_fxaa == true)
		{
			core::dimension2du rrtSize = m_rtt[colorID]->getSize();
			float params[2];
			params[0] = 1.0f / (float)rrtSize.Width;
			params[1] = 1.0f / (float)rrtSize.Height;
			m_fxaaFilter->setUniform2("uRCPFrame", params);
			m_fxaaFilter->setTexture(0, m_rtt[colorID]);
			m_fxaaFilter->applyMaterial(m_effectPass);

			CShaderMaterial::setMaterial(m_fxaaFilter);

			if (m_bloomEffect)
				driver->setRenderTarget(m_rtt[1], false, false);
			else
				driver->setRenderTarget(finalTarget, false, false);

			beginRender2D(renderW, renderH);
			renderBufferToTarget(0.0f, 0.0f, renderW, renderH, m_effectPass);

			colorID = 1;
		}

		if (m_bloomEffect)
		{
			brightFilter(colorID, !colorID);
			blurDown(!colorID, 2);
			blurDown(2, 3);
			blurUp(3, 2);

			// bloom
			m_bloomFilter->setTexture(0, m_rtt[colorID]);
			m_bloomFilter->setTexture(1, m_rtt[2]);
			m_bloomFilter->applyMaterial(m_effectPass);

			CShaderMaterial::setMaterial(m_bloomFilter);

			driver->setRenderTarget(finalTarget, false, false);
			beginRender2D(renderW, renderH);
			renderBufferToTarget(0.0f, 0.0f, renderW, renderH, m_effectPass);
		}

		m_lumTarget = !m_lumTarget;

		// test to target
		/*
		ITexture *tex = m_rtt[2];
		SMaterial t;
		t.setTexture(0, tex);
		t.MaterialType = CShaderManager::getInstance()->getShaderIDByName("TextureColor");

		float w = (float)tex->getSize().Width;
		float h = (float)tex->getSize().Height;

		driver->setRenderTarget(finalTarget, false, false);
		beginRender2D(renderW, renderH);
		renderBufferToTarget(0.0f, 0.0f, renderW, renderH, 0.0f, 0.0f, w, h, t);
		*/
	}

	void CPostProcessorRP::renderEffect(int fromTarget, int toTarget, CMaterial *material)
	{
		IVideoDriver *driver = getVideoDriver();

		driver->setRenderTarget(m_rtt[toTarget]);

		material->setTexture(0, m_rtt[fromTarget]);
		material->applyMaterial(m_effectPass);

		CShaderMaterial::setMaterial(material);

		const core::dimension2du &toSize = m_rtt[toTarget]->getSize();
		const core::dimension2du &fromSize = m_rtt[fromTarget]->getSize();

		beginRender2D((float)toSize.Width, (float)toSize.Height);
		renderBufferToTarget(0, 0, (float)fromSize.Width, (float)fromSize.Height, m_effectPass);
	}
}
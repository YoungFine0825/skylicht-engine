/*
!@
MIT License

Copyright (c) 2019 Skylicht Technology CO., LTD

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
#include "CDeferredRP.h"
#include "Material/CMaterial.h"
#include "Material/Shader/CShaderManager.h"
#include "Material/Shader/ShaderCallback/CShaderShadow.h"
#include "Material/Shader/ShaderCallback/CShaderLighting.h"
#include "Lighting/CLightCullingSystem.h"
#include "Lighting/CPointLight.h"

namespace Skylicht
{
	CDeferredRP::CDeferredRP() :
		m_albedo(NULL),
		m_position(NULL),
		m_normal(NULL),
		m_data(NULL)
	{
		m_type = IRenderPipeline::Deferred;
	}

	CDeferredRP::~CDeferredRP()
	{
		IVideoDriver *driver = getVideoDriver();

		if (m_albedo != NULL)
			driver->removeTexture(m_albedo);

		if (m_position != NULL)
			driver->removeTexture(m_position);

		if (m_normal != NULL)
			driver->removeTexture(m_normal);

		if (m_data != NULL)
			driver->removeTexture(m_data);

		if (m_lightBuffer1 != NULL)
			driver->removeTexture(m_lightBuffer1);

		if (m_lightBuffer2 != NULL)
			driver->removeTexture(m_lightBuffer2);

		m_multiRenderTarget.clear();

	}

	void CDeferredRP::initRender(int w, int h)
	{
		IVideoDriver *driver = getVideoDriver();

		// init render target
		m_size = core::dimension2du((u32)w, (u32)h);
		m_albedo = driver->addRenderTargetTexture(m_size, "albedo", ECF_A8R8G8B8);
		m_position = driver->addRenderTargetTexture(m_size, "position", ECF_A32B32G32R32F);
		m_normal = driver->addRenderTargetTexture(m_size, "normal", ECF_A32B32G32R32F);
		m_data = driver->addRenderTargetTexture(m_size, "data", ECF_A8R8G8B8);

		m_lightBuffer1 = driver->addRenderTargetTexture(m_size, "light_01", ECF_A8R8G8B8);
		m_lightBuffer2 = driver->addRenderTargetTexture(m_size, "light_02", ECF_A8R8G8B8);

		// setup multi render target
		m_multiRenderTarget.push_back(m_albedo);
		m_multiRenderTarget.push_back(m_position);
		m_multiRenderTarget.push_back(m_normal);
		m_multiRenderTarget.push_back(m_data);

		// setup material
		initDefferredMaterial();
		initPointLightMaterial();
	}

	void CDeferredRP::initDefferredMaterial()
	{
		m_directionalLightPass.MaterialType = CShaderManager::getInstance()->getShaderIDByName("SGDirectionalLighting");

		m_directionalLightPass.setTexture(0, m_albedo);
		m_directionalLightPass.setTexture(1, m_position);
		m_directionalLightPass.setTexture(2, m_normal);
		m_directionalLightPass.setTexture(3, m_data);

		// turn off mipmap on float texture	
		m_directionalLightPass.TextureLayer[1].BilinearFilter = false;
		m_directionalLightPass.TextureLayer[1].TrilinearFilter = false;
		m_directionalLightPass.TextureLayer[1].AnisotropicFilter = 0;

		m_directionalLightPass.TextureLayer[2].BilinearFilter = false;
		m_directionalLightPass.TextureLayer[2].TrilinearFilter = false;
		m_directionalLightPass.TextureLayer[2].AnisotropicFilter = 0;

		m_directionalLightPass.TextureLayer[4].BilinearFilter = false;
		m_directionalLightPass.TextureLayer[4].TrilinearFilter = false;
		m_directionalLightPass.TextureLayer[4].AnisotropicFilter = 0;

		// disable Z
		m_directionalLightPass.ZBuffer = video::ECFN_DISABLED;
		m_directionalLightPass.ZWriteEnable = false;
	}

	void CDeferredRP::initPointLightMaterial()
	{
		m_pointLightPass.MaterialType = CShaderManager::getInstance()->getShaderIDByName("SGPointLighting");

		m_pointLightPass.setTexture(0, m_albedo);
		m_pointLightPass.setTexture(1, m_position);
		m_pointLightPass.setTexture(2, m_normal);
		m_pointLightPass.setTexture(3, m_data);

		// turn off mipmap on float texture	
		m_pointLightPass.TextureLayer[0].BilinearFilter = false;
		m_pointLightPass.TextureLayer[0].TrilinearFilter = false;
		m_pointLightPass.TextureLayer[0].AnisotropicFilter = 0;

		m_pointLightPass.TextureLayer[1].BilinearFilter = false;
		m_pointLightPass.TextureLayer[1].TrilinearFilter = false;
		m_pointLightPass.TextureLayer[1].AnisotropicFilter = 0;

		m_pointLightPass.TextureLayer[2].BilinearFilter = false;
		m_pointLightPass.TextureLayer[2].TrilinearFilter = false;
		m_pointLightPass.TextureLayer[2].AnisotropicFilter = 0;

		m_pointLightPass.TextureLayer[3].BilinearFilter = false;
		m_pointLightPass.TextureLayer[3].TrilinearFilter = false;
		m_pointLightPass.TextureLayer[3].AnisotropicFilter = 0;

		// disable Z
		m_pointLightPass.ZBuffer = video::ECFN_DISABLED;
		m_pointLightPass.ZWriteEnable = false;
	}

	bool CDeferredRP::canRenderMaterial(CMaterial *material)
	{
		if (material->isDeferred() == true)
			return true;

		return false;
	}

	void CDeferredRP::render(ITexture *target, CCamera *camera, CEntityManager *entityManager)
	{
		if (camera == NULL)
			return;

		// set multi rtt
		IVideoDriver *driver = getVideoDriver();
		driver->setRenderTarget(m_multiRenderTarget);

		// draw entity to buffer
		setCamera(camera);
		entityManager->setCamera(camera);
		entityManager->setRenderPipeline(this);

		if (m_updateEntity == true)
		{
			entityManager->update();
			entityManager->render();
		}
		else
		{
			entityManager->cullingAndRender();
		}

		// save camera transform
		m_projectionMatrix = driver->getTransform(video::ETS_PROJECTION);
		m_viewMatrix = driver->getTransform(video::ETS_VIEW);

		float renderW = (float)m_size.Width;
		float renderH = (float)m_size.Height;


		// render light pass, clear black color
		driver->setRenderTarget(m_lightBuffer1, true, false);
		driver->setRenderTarget(m_lightBuffer2, true, false);

		ITexture *lightBuffer = NULL;

		CLightCullingSystem *lightCullingSystem = entityManager->getSystem<CLightCullingSystem>();
		if (lightCullingSystem != NULL)
		{
			core::array<CLightCullingData*>& listLight = lightCullingSystem->getLightVisible();
			for (u32 i = 0, n = listLight.size(); i < n; i++)
			{
				CLight *light = listLight[i]->Light;
				CPointLight *pointLight = dynamic_cast<CPointLight*>(light);
				if (pointLight != NULL)
				{
					if (lightBuffer == NULL || lightBuffer == m_lightBuffer2)
					{
						lightBuffer = m_lightBuffer1;
						m_pointLightPass.setTexture(4, m_lightBuffer2);
					}
					else
					{
						lightBuffer = m_lightBuffer2;
						m_pointLightPass.setTexture(4, m_lightBuffer1);
					}

					CShaderLighting::setPointLight(pointLight);

					// render current point light to light buffer
					driver->setRenderTarget(lightBuffer, true, false);
					beginRender2D(renderW, renderH);
					renderBufferToTarget(0.0f, 0.0f, renderW, renderH, m_pointLightPass);
				}
			}
		}


		// render final light to screen
		driver->setRenderTarget(target, true, false);

		// point light color
		m_directionalLightPass.setTexture(4, lightBuffer);

		// shadow
		CShadowMapRP *shadowRP = CShaderShadow::getShadowMapRP();
		if (shadowRP != NULL)
			m_directionalLightPass.setTexture(5, shadowRP->getDepthTexture());

		beginRender2D(renderW, renderH);
		renderBufferToTarget(0.0f, 0.0f, renderW, renderH, m_directionalLightPass);

		onNext(target, camera, entityManager);
	}
}
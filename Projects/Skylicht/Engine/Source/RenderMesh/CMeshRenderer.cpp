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
#include "CMeshRenderer.h"

#include "Culling/CCullingData.h"
#include "Entity/CEntityManager.h"

#include "Material/Shader/ShaderCallback/CShaderSH.h"
#include "Material/Shader/ShaderCallback/CShaderLighting.h"

namespace Skylicht
{
	CMeshRenderer::CMeshRenderer()
	{
		m_pipelineType = IRenderPipeline::Mix;
	}

	CMeshRenderer::~CMeshRenderer()
	{

	}

	void CMeshRenderer::beginQuery(CEntityManager* entityManager)
	{
		m_meshs.set_used(0);
		m_transforms.set_used(0);
		m_indirectLightings.set_used(0);
	}

	void CMeshRenderer::onQuery(CEntityManager* entityManager, CEntity* entity)
	{
		CRenderMeshData* meshData = (CRenderMeshData*)entity->getDataByIndex(CRenderMeshData::DataTypeIndex);
		if (meshData != NULL)
		{
			if (meshData->getMesh() == NULL)
				return;

			// do not render gpu skinning, pass for CSkinMeshRenderer
			if (meshData->isSkinnedMesh() == true && meshData->isSoftwareSkinning() == false)
				return;

			bool cullingVisible = true;

			// get culling result from CCullingSystem
			CCullingData* cullingData = (CCullingData*)entity->getDataByIndex(CCullingData::DataTypeIndex);
			if (cullingData != NULL)
				cullingVisible = cullingData->Visible;

			// only render visible culling mesh
			if (cullingVisible == true)
			{
				m_meshs.push_back(meshData);
			}
		}
	}

	void CMeshRenderer::init(CEntityManager* entityManager)
	{

	}

	int cmpRenderMeshFunc(const void* a, const void* b)
	{
		CRenderMeshData* pa = *((CRenderMeshData**)a);
		CRenderMeshData* pb = *((CRenderMeshData**)b);

		CMesh* meshA = pa->getMesh();
		CMesh* meshB = pb->getMesh();

		std::vector<CMaterial*>& materialsA = meshA->Material;
		std::vector<CMaterial*>& materialsB = meshB->Material;

		// no material, compare by mesh
		if (materialsA.size() == 0 || materialsA.size() == 0)
		{
			if (meshA == meshB)
				return 0;

			return meshA < meshB ? -1 : 1;
		}

		CMaterial* materialA = materialsA[0];
		CMaterial* materialB = materialsB[0];

		// compare by mesh
		if (materialA == NULL || materialB == NULL)
		{
			if (meshA == meshB)
				return 0;

			return meshA < meshB ? -1 : 1;
		}

		// comprate by texture
		ITexture* textureA = materialA->getTexture(0);
		ITexture* textureB = materialB->getTexture(0);

		// if no texture
		if (textureA == NULL || textureB == NULL)
		{
			if (materialA == materialB)
				return 0;
			return materialA < materialB ? -1 : 1;
		}

		// sort by texture 0
		if (textureA == textureB)
			return 0;

		return textureA < textureB ? -1 : 1;
	}

	void CMeshRenderer::update(CEntityManager* entityManager)
	{
		// need sort render by material, texture, mesh		
		u32 count = m_meshs.size();

		// need sort by material
		qsort(m_meshs.pointer(), count, sizeof(CRenderMeshData*), cmpRenderMeshFunc);

		// get world transform			
		for (u32 i = 0; i < count; i++)
		{
			CEntity* entity = entityManager->getEntity(m_meshs[i]->EntityIndex);

			CWorldTransformData* transform = (CWorldTransformData*)entity->getDataByIndex(CWorldTransformData::DataTypeIndex);
			CIndirectLightingData* indirect = (CIndirectLightingData*)entity->getDataByIndex(CIndirectLightingData::DataTypeIndex);

			m_transforms.push_back(transform);
			m_indirectLightings.push_back(indirect);
		}
	}

	void CMeshRenderer::render(CEntityManager* entityManager)
	{
		IVideoDriver* driver = getVideoDriver();

		CRenderMeshData** meshs = m_meshs.pointer();
		CWorldTransformData** transforms = m_transforms.pointer();
		CIndirectLightingData** indirectLighting = m_indirectLightings.pointer();

		IRenderPipeline* rp = entityManager->getRenderPipeline();

		for (u32 i = 0, n = m_meshs.size(); i < n; i++)
		{
			CRenderMeshData* meshData = m_meshs[i];
			CMesh* mesh = meshData->getMesh();

			CIndirectLightingData* lightingData = indirectLighting[i];
			if (lightingData != NULL)
			{
				if (lightingData->Type == CIndirectLightingData::SH9)
					CShaderSH::setSH9(lightingData->SH);
				else if (lightingData->Type == CIndirectLightingData::AmbientColor)
					CShaderLighting::setLightAmbient(lightingData->Color);
			}

			driver->setTransform(video::ETS_WORLD, transforms[i]->World);

			for (u32 j = 0, m = mesh->getMeshBufferCount(); j < m; j++)
				rp->drawMeshBuffer(mesh, j, entityManager, meshData->EntityIndex, false);
		}
	}
}
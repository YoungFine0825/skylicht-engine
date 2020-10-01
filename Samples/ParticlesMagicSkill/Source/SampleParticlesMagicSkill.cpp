#include "pch.h"
#include "SkylichtEngine.h"
#include "SampleParticlesMagicSkill.h"
#include "imgui.h"
#include "CImguiManager.h"

#include "GridPlane/CGridPlane.h"
#include "ParticleSystem/CParticleComponent.h"
#include "ParticleSystem/CParticleTrailComponent.h"

void installApplication(const std::vector<std::string>& argv)
{
	SampleParticlesMagicSkill *app = new SampleParticlesMagicSkill();
	getApplication()->registerAppEvent("SampleParticlesMagicSkill", app);
}

SampleParticlesMagicSkill::SampleParticlesMagicSkill() :
	m_scene(NULL)
{
	CImguiManager::createGetInstance();

	CEventManager::getInstance()->registerEvent("App", this);
}

SampleParticlesMagicSkill::~SampleParticlesMagicSkill()
{
	CEventManager::getInstance()->unRegisterEvent(this);

	delete m_scene;
	CImguiManager::releaseInstance();
}

void SampleParticlesMagicSkill::onInitApp()
{
	// init application
	CBaseApp* app = getApplication();

	// app->setClearColor(video::SColor(255, 100, 100, 100));

	// load "BuiltIn.zip" to read files inside it
	app->getFileSystem()->addFileArchive(app->getBuiltInPath("BuiltIn.zip"), false, false);
	app->getFileSystem()->addFileArchive(app->getBuiltInPath("Particles.zip"), false, false);

	// load basic shader
	CShaderManager *shaderMgr = CShaderManager::getInstance();
	shaderMgr->initBasicShader();

	// create a Scene
	m_scene = new CScene();

	// create a Zone in Scene
	CZone *zone = m_scene->createZone();

	// create 2D camera
	CGameObject *guiCameraObject = zone->createEmptyObject();
	m_guiCamera = guiCameraObject->addComponent<CCamera>();
	m_guiCamera->setProjectionType(CCamera::OrthoUI);

	// create 3d camera
	CGameObject *camObj = zone->createEmptyObject();
	camObj->addComponent<CCamera>();
	camObj->addComponent<CEditorCamera>()->setMoveSpeed(2.0f);

	m_camera = camObj->getComponent<CCamera>();
	m_camera->setPosition(core::vector3df(0.0f, 1.5f, 4.0f));
	m_camera->lookAt(core::vector3df(0.0f, 0.0f, 0.0f), core::vector3df(0.0f, 1.0f, 0.0f));

	// 3d grid
	CGameObject *grid = zone->createEmptyObject();
	grid->addComponent<CGridPlane>();

	// particles pool
	for (int i = 0; i < MAX_POOL; i++)
	{
		m_particlePool[i] = zone->createEmptyObject()->addComponent<Particle::CParticleComponent>();
		initParticleSystem(m_particlePool[i]);
		m_particlePool[i]->getGameObject()->setVisible(false);
	}

	// init font
	CGlyphFreetype *freetypeFont = CGlyphFreetype::getInstance();
	freetypeFont->initFont("Segoe UI Light", "BuiltIn/Fonts/segoeui/segoeuil.ttf");

	m_font = new CGlyphFont();
	m_font->setFont("Segoe UI Light", 30);

	// create 2D Canvas
	CGameObject *canvasObject = zone->createEmptyObject();
	CCanvas *canvas = canvasObject->addComponent<CCanvas>();

	// create UI Text in Canvas
	CGUIText *textLarge = canvas->createText(m_font);
	textLarge->setPosition(core::vector3df(0.0f, -10.0f, 0.0f));
	textLarge->setText("Press SPACE to simulate explosion");
	textLarge->setTextAlign(CGUIElement::Center, CGUIElement::Bottom);

	// rendering pipe line
	m_forwardRP = new CForwardRP();
}

bool SampleParticlesMagicSkill::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT)
	{
		if (event.KeyInput.Key == irr::KEY_SPACE && event.KeyInput.PressedDown == false)
		{
			for (int i = 0; i < MAX_POOL; i++)
			{
				if (m_particlePool[i]->IsPlaying() == false)
				{
					char debugLog[512];
					sprintf(debugLog, "Play particle %d", i);
					os::Printer::log(debugLog);

					float radius = 2.0f;
					float r1 = Particle::random(-1.0f, 1.0f);
					float r2 = Particle::random(0.0f, 1.0f);
					core::vector3df randomPosition(r1 * radius, r2 * radius, r1 * radius);

					m_particlePool[i]->getGameObject()->setVisible(true);
					m_particlePool[i]->getGameObject()->getTransformEuler()->setPosition(randomPosition);
					m_particlePool[i]->Play();

					return true;
				}
			}
		}
	}

	return false;
}

void SampleParticlesMagicSkill::initParticleSystem(Particle::CParticleComponent *ps)
{
	ITexture *texture = NULL;

	// FACTORY & ZONE
	Particle::CFactory *factory = ps->getParticleFactory();
	Particle::CCylinder* cylinder = factory->createCylinderZone(core::vector3df(), core::vector3df(0.0f, 1.0f, 0.0f), 0.6, 0.1f);

	// GROUP: SPARK
	Particle::CGroup *sparkGroup = ps->createParticleGroup();

	sparkGroup->LifeMin = 4.0f;
	sparkGroup->LifeMax = 8.0f;
	sparkGroup->Friction = 0.4f;
	sparkGroup->Gravity.set(0.0f, -0.1f, 0.0f);
	sparkGroup->createModel(Particle::ColorA)->setStart(1.0f)->setEnd(0.0f);

	Particle::CNormalEmitter *sparkEmitter = factory->createNormalEmitter(false);
	sparkEmitter->setFlow(100.0f);
	sparkEmitter->setTank(1);
	sparkEmitter->setForce(2.5f, 6.0f);
	sparkEmitter->setZone(cylinder);
	sparkGroup->addEmitter(sparkEmitter);

	// ADD TRAIL
	Particle::CParticleTrailComponent *psTrail = ps->getGameObject()->addComponent<Particle::CParticleTrailComponent>();
	Particle::CParticleTrail *trail = psTrail->addTrail(sparkGroup);

	CMaterial *material = trail->getMaterial();
	material->setTexture(0, CTextureManager::getInstance()->getTexture("Particles/Textures/Arcane/arcane_trail.png"));
	trail->applyMaterial();
	trail->setWidth(0.5f);


	// SUB GROUP: Arcane
	Particle::CSubGroup *arcaneGroup = ps->createParticleSubGroup(sparkGroup);

	Particle::CQuadRenderer *pointSpark = factory->createQuadRenderer();
	pointSpark->SizeX = 1.0f;
	pointSpark->SizeY = 1.0f;
	pointSpark->SizeZ = 1.0f;
	arcaneGroup->setRenderer(pointSpark);

	texture = CTextureManager::getInstance()->getTexture("Particles/Textures/Arcane/arcane_twirl.png");
	pointSpark->setMaterialType(Particle::Addtive, Particle::Camera);
	pointSpark->getMaterial()->setTexture(0, texture);
	pointSpark->getMaterial()->applyMaterial();

	arcaneGroup->createModel(Particle::ColorA)->setStart(1.0f)->setEnd(0.0f);
	arcaneGroup->createModel(Particle::RotateSpeedZ)->setStart(3.0f, 5.0f);

	Particle::CNormalEmitter *pointSparkEmitter = factory->createNormalEmitter(false);
	pointSparkEmitter->setFlow(100.0f);
	pointSparkEmitter->setTank(1);
	pointSparkEmitter->setForce(0.0f, 0.0f);
	pointSparkEmitter->setZone(cylinder);
	arcaneGroup->addEmitter(pointSparkEmitter);
}

void SampleParticlesMagicSkill::onUpdate()
{
	// update application
	m_scene->update();

	// imgui update
	CImguiManager::getInstance()->onNewFrame();
}

void SampleParticlesMagicSkill::onRender()
{
	// render 3d scene
	m_forwardRP->render(NULL, m_camera, m_scene->getEntityManager(), core::recti());

	// Render 2d gui camera
	CGraphics2D::getInstance()->render(m_guiCamera);

	CImguiManager::getInstance()->onRender();
}

void SampleParticlesMagicSkill::onPostRender()
{
	// post render application
}

bool SampleParticlesMagicSkill::onBack()
{
	// on back key press
	// return TRUE will run default by OS (Mobile)
	// return FALSE will cancel BACK FUNCTION by OS (Mobile)
	return true;
}

void SampleParticlesMagicSkill::onResume()
{
	// resume application
}

void SampleParticlesMagicSkill::onPause()
{
	// pause application
}

void SampleParticlesMagicSkill::onQuitApp()
{
	// end application
	delete this;
}
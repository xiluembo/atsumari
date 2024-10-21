/*
 * Atsumari: Roll your Twitch Chat Emotes and Emojis in a ball
 * Copyright (C) 2024 - Andrius da Costa Ribas <andriusmao@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "atsumariwindow.h"

#include <QMouseEvent>
#include <Qt3DRender/QViewport>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QRenderSurfaceSelector>
#include <Qt3DRender/QRenderStateSet>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QMaterial>
#include <Qt3DExtras/QGoochMaterial>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DRender/QDepthTest>

AtsumariWindow::AtsumariWindow() : Qt3DExtras::Qt3DWindow() {
    Qt3DRender::QCamera *camera = this->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(20.0, 20.0, 20.0f));
    camera->setViewCenter(QVector3D(0, 0, 0));

    Qt3DRender::QRenderSurfaceSelector *surfaceSelector = new Qt3DRender::QRenderSurfaceSelector;
    surfaceSelector->setSurface(this);
    Qt3DRender::QViewport *viewport = new Qt3DRender::QViewport(surfaceSelector);
    viewport->setNormalizedRect(QRectF(0, 0, 1.0, 1.0));
    Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector(viewport);
    cameraSelector->setCamera(camera);
    Qt3DRender::QClearBuffers *clearBuffers = new Qt3DRender::QClearBuffers(cameraSelector);
    clearBuffers->setBuffers(Qt3DRender::QClearBuffers::ColorDepthBuffer);
    clearBuffers->setClearColor(Qt::black);
    Qt3DRender::QRenderStateSet *renderStateSet = new Qt3DRender::QRenderStateSet(clearBuffers);
    Qt3DRender::QCullFace *cullFace = new Qt3DRender::QCullFace(renderStateSet);
    cullFace->setMode(Qt3DRender::QCullFace::NoCulling);
    renderStateSet->addRenderState(cullFace);
    Qt3DRender::QDepthTest *depthTest = new Qt3DRender::QDepthTest;
    depthTest->setDepthFunction(Qt3DRender::QDepthTest::Less);
    renderStateSet->addRenderState(depthTest);
    setActiveFrameGraph(surfaceSelector);

    Qt3DCore::QEntity *root = createScene();
    setRootEntity(root);
}

Qt3DCore::QEntity* AtsumariWindow::createScene() { // Root entity
    Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity;

    Qt3DCore::QEntity *planeEntity = new Qt3DCore::QEntity(rootEntity);
    Qt3DRender::QMaterial *meshMaterial = new Qt3DExtras::QGoochMaterial;
    Qt3DExtras::QPlaneMesh *planeMesh = new Qt3DExtras::QPlaneMesh;
    planeMesh->setHeight(10);
    planeMesh->setWidth(10);
    planeTransform = new Qt3DCore::QTransform;

    planeEntity->addComponent(planeTransform);
    planeEntity->addComponent(planeMesh);
    planeEntity->addComponent(meshMaterial);

    return rootEntity;
}

//
//  ShapeManager.cpp
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.10.29
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ShapeManager.h"

#include <glm/gtx/norm.hpp>

#include <QDebug>

#include "ShapeFactory.h"

ShapeManager::ShapeManager() {
}

ShapeManager::~ShapeManager() {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        ShapeFactory::deleteShape(shapeRef->shape);
    }
    _shapeMap.clear();
}

const btCollisionShape* ShapeManager::getShape(const ShapeInfo& info) {
    if (info.getType() == SHAPE_TYPE_NONE) {
        return nullptr;
    }
    HashKey key = info.getHash();
    ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        shapeRef->refCount++;
        return shapeRef->shape;
    }
    const btCollisionShape* shape = ShapeFactory::createShapeFromInfo(info);
    if (shape) {
        ShapeReference newRef;
        newRef.refCount = 1;
        newRef.shape = shape;
        newRef.key = key;
        _shapeMap.insert(key, newRef);
    }
    return shape;
}

// private helper method
bool ShapeManager::releaseShapeByKey(const HashKey& key) {
    ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        if (shapeRef->refCount > 0) {
            shapeRef->refCount--;
            if (shapeRef->refCount == 0) {
                _pendingGarbage.push_back(key);
                const int MAX_SHAPE_GARBAGE_CAPACITY = 255;
                if (_pendingGarbage.size() > MAX_SHAPE_GARBAGE_CAPACITY) {
                    collectGarbage();
                }
            }
            return true;
        } else {
            // attempt to remove shape that has no refs
            assert(false);
        }
    } else {
        // attempt to remove unmanaged shape
        assert(false);
    }
    return false;
}

bool ShapeManager::releaseShape(const btCollisionShape* shape) {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        if (shape == shapeRef->shape) {
            return releaseShapeByKey(shapeRef->key);
        }
    }
    return false;
}

void ShapeManager::collectGarbage() {
    int numShapes = _pendingGarbage.size();
    for (int i = 0; i < numShapes; ++i) {
        HashKey& key = _pendingGarbage[i];
        ShapeReference* shapeRef = _shapeMap.find(key);
        if (shapeRef && shapeRef->refCount == 0) {
            ShapeFactory::deleteShape(shapeRef->shape);
            _shapeMap.remove(key);
        }
    }
    _pendingGarbage.clear();
}

int ShapeManager::getNumReferences(const ShapeInfo& info) const {
    HashKey key = info.getHash();
    const ShapeReference* shapeRef = _shapeMap.find(key);
    if (shapeRef) {
        return shapeRef->refCount;
    }
    return 0;
}

int ShapeManager::getNumReferences(const btCollisionShape* shape) const {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        const ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        if (shape == shapeRef->shape) {
            return shapeRef->refCount;
        }
    }
    return 0;
}

bool ShapeManager::hasShape(const btCollisionShape* shape) const {
    int numShapes = _shapeMap.size();
    for (int i = 0; i < numShapes; ++i) {
        const ShapeReference* shapeRef = _shapeMap.getAtIndex(i);
        if (shape == shapeRef->shape) {
            return true;
        }
    }
    return false;
}

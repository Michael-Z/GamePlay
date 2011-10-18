/*
 * ParticleEmitter.cpp
 */

#include "Base.h"
#include "ParticleEmitter.h"
#include "Game.h"
#include "Node.h"
#include "Quaternion.h"
#include "Properties.h"

#define PARTICLEEMITTER_EMISSION_RATE_DEFAULT 10

namespace gameplay
{

ParticleEmitter::ParticleEmitter(unsigned int particleCountMax, SpriteBatch* batch) :
    _particleCountMax(particleCountMax), _particleCount(0), _particles(NULL),
    _spriteBatch(batch), _node(NULL), _active(false),
    _orbitPosition(false), _orbitVelocity(false), _orbitAcceleration(false),
    _ellipsoid(false),
    _sizeStartMin(1.0f), _sizeStartMax(1.0f), _sizeEndMin(1.0f), _sizeEndMax(1.0f),
    _energyMin(1000L), _energyMax(1000L),
    _angularVelocityMin(0.0f), _angularVelocityMax(0.0f),
    _colorStart(Color::white()), _colorStartVar(Color::empty()), _colorEnd(Color::white()), _colorEndVar(Color::empty()),
    _position(Vector3::zero()), _positionVar(Vector3::zero()),
    _velocity(Vector3::zero()), _velocityVar(Vector3::one()),
    _acceleration(Vector3::zero()), _accelerationVar(Vector3::zero()),
    _emissionRate(PARTICLEEMITTER_EMISSION_RATE_DEFAULT),
    _timePerEmission(0L), _timeLast(0L), _timeRunning(0L),
    _spriteFrameCount(1), _spriteFrameDuration(0L), _spriteFrameDurationSecs(0.0f), _spritePercentPerFrame(0.0f),
    _spriteRandomOffset(0), _spriteLooped(false), _spriteAnimating(false),
    _texCoords(NULL), _textureWidth(0), _textureHeight(0), _textureWidthRatio(0), _textureHeightRatio(0)
{
    _particles = new Particle[particleCountMax];
    _timePerEmission = 1000L / PARTICLEEMITTER_EMISSION_RATE_DEFAULT;
}

ParticleEmitter::~ParticleEmitter()
{
    SAFE_RELEASE(_node);

    if (_particles)
    {
        delete _particles;
        _particles = NULL;
    }
    if (_texCoords)
    {
        delete _texCoords;
        _texCoords = NULL;
    }
    if (_spriteBatch)
    {
        delete _spriteBatch;
        _spriteBatch = NULL;
    }
}

ParticleEmitter* ParticleEmitter::create(const char* textureFile, unsigned int particleCountMax)
{
    assert(textureFile);

    Texture* texture = NULL;
    texture = Texture::create(textureFile, true);    

    if (!texture)
    {
        // Use default texture.
        texture = Texture::create("../gameplay-resources/res/textures/particle-default.png", true);
    }
    assert(texture);

    // Use default SpriteBatch material.
    SpriteBatch* batch =  SpriteBatch::create(texture, NULL, particleCountMax);
    assert(batch);

    ParticleEmitter* emitter = new ParticleEmitter(particleCountMax, batch);
    assert(emitter);

    // By default assume only one frame which uses the entire texture.
    emitter->_textureWidth = texture->getWidth();
    emitter->_textureHeight = texture->getHeight();
    emitter->_textureWidthRatio = 1.0f / (float)texture->getWidth();
    emitter->_textureHeightRatio = 1.0f / (float)texture->getHeight();
    emitter->setSpriteFrameCoords(1, new Rectangle(texture->getWidth(), texture->getHeight()));

    return emitter;
}

ParticleEmitter* ParticleEmitter::create(const char* particleFile)
{
    assert(particleFile);

    Properties* properties = Properties::create(particleFile);
    if (!properties)
    {
        return NULL;
    }

    std::string spriteTexture = properties->getString("spriteTexture");

    unsigned int particleCountMax = (unsigned int)properties->getInt("particleCountMax");
    unsigned int emissionRate = (unsigned int)properties->getInt("emissionRate");
    int sfactor = properties->getInt("sfactor");
    int dfactor = properties->getInt("dfactor");
    
    bool orbitPosition = properties->getBool("orbitPosition");
    bool orbitVelocity = properties->getBool("orbitVelocity");
    bool orbitAcceleration = properties->getBool("orbitAcceleration");
    bool ellipsoid = properties->getBool("ellipsoid");

    float sizeStartMin = properties->getFloat("sizeStartMin");
    float sizeStartMax = properties->getFloat("sizeStartMax");
    float sizeEndMin = properties->getFloat("sizeEndMin");
    float sizeEndMax = properties->getFloat("sizeEndMax");
    long energyMin = properties->getLong("energyMin");
    long energyMax = properties->getLong("energyMax");
    float angularVelocityMin = properties->getFloat("angularVelocityMin");
    float angularVelocityMax = properties->getFloat("angularVelocityMax");

    Color colorStart;
    Color colorStartVar;
    Color colorEnd;
    Color colorEndVar;
    properties->getColor("colorStart", &colorStart);
    properties->getColor("colorStartVar", &colorStartVar);
    properties->getColor("colorEnd", &colorEnd);
    properties->getColor("colorEndVar", &colorEndVar);

    Vector3 position;
    Vector3 positionVar;
    Vector3 velocity;
    Vector3 velocityVar;
    Vector3 acceleration;
    Vector3 accelerationVar;
    properties->getVector3("position", &position);
    properties->getVector3("positionVar", &positionVar);
    properties->getVector3("velocity", &velocity);
    properties->getVector3("velocityVar", &velocityVar);
    properties->getVector3("acceleration", &acceleration);
    properties->getVector3("accelerationVar", &accelerationVar);
    
    int spriteFrameCount = properties->getInt("spriteFrameCount");
    float spriteFrameDuration = properties->getFloat("spriteFrameDuration");
    int spriteWidth = properties->getInt("spriteWidth");
    int spriteHeight = properties->getInt("spriteHeight");
    int spriteRandomOffset = properties->getInt("spriteRandomOffset");
    bool spriteLooped = properties->getBool("spriteLooped");
    bool spriteAnimating = properties->getBool("spriteAnimating");

    ParticleEmitter* emitter = ParticleEmitter::create(spriteTexture.c_str(), particleCountMax);
    emitter->setEmissionRate(emissionRate);
    emitter->setOrbitAroundOrigin(orbitPosition, orbitVelocity, orbitAcceleration);
    emitter->setEllipsoid(ellipsoid);
    emitter->setSize(sizeStartMin, sizeStartMax, sizeEndMin, sizeEndMax);
    emitter->setEnergy(energyMin, energyMax);
    emitter->setAngularVelocity(angularVelocityMin, angularVelocityMax);
    emitter->setColor(colorStart, colorStartVar, colorEnd, colorEndVar);
    emitter->setPosition(position, positionVar);
    emitter->setVelocity(velocity, velocityVar);
    emitter->setAcceleration(acceleration, accelerationVar);

    emitter->setSpriteAnimating(spriteAnimating);
    emitter->setSpriteLooped(spriteLooped);
    emitter->setSpriteRandomOffset(spriteRandomOffset);
    emitter->setSpriteFrameDuration(spriteFrameDuration);
    emitter->setSpriteFrameCoords(spriteFrameCount, spriteWidth, spriteHeight);

    emitter->setBlendMode(sfactor, dfactor);

    //Properties::destroy(properties);
    delete properties;
    properties = NULL;

    return emitter;
}

Node* ParticleEmitter::getNode() const
{
    return _node;
}

void ParticleEmitter::setNode(Node* node)
{
    if (_node != node)
    {
        // Disconnect our current transform.
        SAFE_RELEASE(_node);

        // Connect the new transform.
        _node = node;

        if (_node)
        {
            _node->addRef();

            // Bind this node's view projection matrix to this emitter's material.
            _spriteBatch->getMaterial()->getParameter("sb_ortho_projection")->bindValue(node, &Node::getViewProjectionMatrix);
        }
    }
}

unsigned int ParticleEmitter::getParticlesCount() const
{
    return _particleCount;
}

void ParticleEmitter::setOrbitAroundOrigin(bool orbitPosition, bool orbitVelocity, bool orbitAcceleration)
{
    _orbitPosition = orbitPosition;
    _orbitVelocity = orbitVelocity;
    _orbitAcceleration = orbitAcceleration;
}

float ParticleEmitter::getSizeStartMin() const
{
    return _sizeStartMin;
}

float ParticleEmitter::getSizeStartMax() const
{
    return _sizeStartMax;
}

float ParticleEmitter::getSizeEndMin() const
{
    return _sizeEndMin;
}

float ParticleEmitter::getSizeEndMax() const
{
    return _sizeEndMax;
}

void ParticleEmitter::setSize(float startMin, float startMax, float endMin, float endMax)
{
    _sizeStartMin = startMin;
    _sizeStartMax = startMax;
    _sizeEndMin = endMin;
    _sizeEndMax = endMax;
}

long ParticleEmitter::getEnergyMin() const
{
    return _energyMin;
}

long ParticleEmitter::getEnergyMax() const
{
    return _energyMax;
}

void ParticleEmitter::setEnergy(long min, long max)
{
    _energyMin = min;
    _energyMax = max;
}

const Color& ParticleEmitter::getColorStart() const
{
    return _colorStart;
}

const Color& ParticleEmitter::getColorStartVariance() const
{
    return _colorStartVar;
}

const Color& ParticleEmitter::getColorEnd() const
{
    return _colorEnd;
}

const Color& ParticleEmitter::getColorEndVariance() const
{
    return _colorEndVar;
}

void ParticleEmitter::setColor(const Color& startColor, const Color& startColorVar, const Color& endColor, const Color& endColorVar)
{
    _colorStart.set(startColor);
    _colorStartVar.set(startColorVar);
    _colorEnd.set(endColor);
    _colorEndVar.set(endColorVar);
}

const Vector3& ParticleEmitter::getPosition() const
{
    return _position;
}

const Vector3& ParticleEmitter::getPositionVariance() const
{
    return _positionVar;
}

void ParticleEmitter::setPosition(const Vector3& position, const Vector3& positionVar)
{
    _position.set(position);
    _positionVar.set(positionVar);
}

const Vector3& ParticleEmitter::getVelocity() const
{
    return _velocity;
}

const Vector3& ParticleEmitter::getVelocityVariance() const
{
    return _velocityVar;
}

void ParticleEmitter::setVelocity(const Vector3& velocity, const Vector3& velocityVar)
{
    _velocity.set(velocity);
    _velocityVar.set(velocityVar);
}

float ParticleEmitter::getAngularVelocityMin() const
{
    return _angularVelocityMin;
}

float ParticleEmitter::getAngularVelocityMax() const
{
    return _angularVelocityMax;
}

void ParticleEmitter::setAngularVelocity(float min, float max)
{
    _angularVelocityMin = min;
    _angularVelocityMax = max;
}

const Vector3& ParticleEmitter::getAcceleration() const
{
    return _acceleration;
}

const Vector3& ParticleEmitter::getAccelerationVariance() const
{
    return _accelerationVar;
}

void ParticleEmitter::setAcceleration(const Vector3& acceleration, const Vector3& accelerationVar)
{
    _acceleration.set(acceleration);
    _accelerationVar.set(accelerationVar);
}

unsigned int ParticleEmitter::getEmissionRate() const
{
    return _emissionRate;
}

void ParticleEmitter::setEmissionRate(unsigned int rate)
{
    _emissionRate = rate;
    _timePerEmission = 1000L / _emissionRate;
}

void ParticleEmitter::setBlendMode(const GLenum sfactor, const GLenum dfactor)
{
    _spriteBatch->setBlendMode(sfactor, dfactor);
}

void ParticleEmitter::setSpriteRandomOffset(int maxOffset)
{
    _spriteRandomOffset = maxOffset;
}

void ParticleEmitter::setSpriteLooped(bool looped)
{
    _spriteLooped = looped;
}

void ParticleEmitter::setSpriteAnimating(bool animating)
{
    _spriteAnimating = animating;
}

void ParticleEmitter::setSpriteFrameDuration(long duration)
{
    _spriteFrameDuration = duration;
    _spriteFrameDurationSecs = (float)duration / 1000.0f;
}

void ParticleEmitter::setEllipsoid(bool ellipsoid)
{
    _ellipsoid = ellipsoid;
}

void ParticleEmitter::setSpriteTexCoords(unsigned int frameCount, float* texCoords)
{
    _spriteFrameCount = frameCount;
    _spritePercentPerFrame = 1.0f / (float)frameCount;

    if (_texCoords)
    {
        delete[] _texCoords;
    }
    _texCoords = new float[frameCount * 4];
    memcpy(_texCoords, texCoords, frameCount * 4 * sizeof(float));
}

void ParticleEmitter::setSpriteFrameCoords(unsigned int frameCount, Rectangle* frameCoords)
{
    _spriteFrameCount = frameCount;
    _spritePercentPerFrame = 1.0f / (float)frameCount;

    float* texCoords = new float[frameCount * 4];

    // Pre-compute texture coordinates from rects.
    for (unsigned int i = 0; i < frameCount; i++)
    {
        float u1 = _textureWidthRatio * frameCoords[i].x;
        float v1 = 1.0f - _textureHeightRatio * frameCoords[i].y;
        float u2 = u1 + _textureWidthRatio * frameCoords[i].width;
        float v2 = v1 - _textureHeightRatio * frameCoords[i].height;

        texCoords[i*4] = u1;
        texCoords[i*4 + 1] = v1;
        texCoords[i*4 + 2] = u2;
        texCoords[i*4 + 3] = v2;
    }

    if (_texCoords)
    {
        delete[] _texCoords;
    }
    _texCoords = new float[frameCount * 4];
    memcpy(_texCoords, texCoords, frameCount * 4 * sizeof(float));

    if (texCoords)
    {
        delete[] texCoords;
        texCoords = NULL;
    }
}

void ParticleEmitter::setSpriteFrameCoords(unsigned int frameCount, int width, int height)
{
    int x;
    int y;
    Rectangle* frameCoords = new Rectangle[frameCount];
    unsigned int cols = _textureWidth / width;
    unsigned int rows = _textureHeight / height;

    unsigned int n = 0;
    for (unsigned int i = 0; i < rows; ++i)
    {
        y = i * height;
        for (unsigned int j = 0; j < cols; ++j)
        {
            x = j * width;
            frameCoords[i*cols + j] = Rectangle(x, y, width, height);
            if (++n == frameCount)
            {
                break;
            }
        }

        if (n == frameCount)
        {
            break;
        }
    }

    setSpriteFrameCoords(frameCount, frameCoords);
}

void ParticleEmitter::start()
{
    _active = true;
    _timeLast = Game::getGameTime();
}

void ParticleEmitter::stop()
{
    _active = false;
}

inline long ParticleEmitter::generateScalar(long min, long max)
{
    // Note: this is not a very good RNG, but it should be suitable for our purposes.
    long r;
    for (unsigned int i = 0; i < sizeof(long)/sizeof(int); i++)
    {
        r = r << 8; // sizeof(int) * CHAR_BITS
        r |= rand();
    }

    // Now we have a random long between 0 and MAX_LONG.  We need to clamp it between min and max.
    r %= max - min;
    r += min;

    return r;
}

inline float ParticleEmitter::generateScalar(float min, float max)
{
    return min + (max - min) * MATH_RANDOM_0_1();
}

inline void ParticleEmitter::generateVectorInRect(const Vector3& base, const Vector3& variance, Vector3* dst)
{
    // Scale each component of the variance vector by a random float
    // between -1 and 1, then add this to the corresponding base component.
    dst->x = base.x + variance.x * MATH_RANDOM_MINUS1_1();
    dst->y = base.y + variance.y * MATH_RANDOM_MINUS1_1();
    dst->z = base.z + variance.z * MATH_RANDOM_MINUS1_1();
}

void ParticleEmitter::generateVectorInEllipsoid(const Vector3& center, const Vector3& scale, Vector3* dst)
{
    // Generate a point within a unit cube, then reject if the point is not in a unit sphere.
    do
    {
        dst->x = MATH_RANDOM_MINUS1_1();
        dst->y = MATH_RANDOM_MINUS1_1();
        dst->z = MATH_RANDOM_MINUS1_1();
    } while (dst->length() > 1.0f);
    
    // Scale this point by the scaling vector.
    dst->x *= scale.x;
    dst->y *= scale.y;
    dst->z *= scale.z;

    // Translate by the center point.
    dst->add(center);
}

inline void ParticleEmitter::generateVector(const Vector3& base, const Vector3& variance, Vector3* dst, bool ellipsoid)
{
    if (ellipsoid)
    {
        generateVectorInEllipsoid(base, variance, dst);
    }
    else
    {
        generateVectorInRect(base, variance, dst);
    }
}

inline void ParticleEmitter::generateColor(const Color& base, const Color& variance, Color* dst)
{
    // Scale each component of the variance color by a random float
    // between -1 and 1, then add this to the corresponding base component.
    dst->r = base.r + variance.r * MATH_RANDOM_MINUS1_1();
    dst->g = base.g + variance.g * MATH_RANDOM_MINUS1_1();
    dst->b = base.b + variance.b * MATH_RANDOM_MINUS1_1();
    dst->a = base.a + variance.a * MATH_RANDOM_MINUS1_1();
}

void ParticleEmitter::emit(unsigned int particleCount)
{
    // Limit particleCount so as not to go over _particleCountMax.
    if (particleCount + _particleCount > _particleCountMax)
    {
        particleCount = _particleCountMax - _particleCount;
    }

    Vector3 translation;
    Matrix world = _node->getWorldMatrix();
    world.getTranslation(&translation);

    // Take translation out of world matrix so it can be used to rotate orbiting properties.
    world.m[12] = 0.0f;
    world.m[13] = 0.0f;
    world.m[14] = 0.0f;

    // Emit the new particles.
    for (unsigned int i = 0; i < particleCount; i++)
    {
        Particle* p = &_particles[_particleCount];

        generateColor(_colorStart, _colorStartVar, &p->_colorStart);
        generateColor(_colorEnd, _colorEndVar, &p->_colorEnd);
        p->_color.set(p->_colorStart);

        p->_energy = p->_energyStart = generateScalar(_energyMin, _energyMax);
        p->_size = p->_sizeStart = generateScalar(_sizeStartMin, _sizeStartMax);
        p->_sizeEnd = generateScalar(_sizeEndMin, _sizeEndMax);
        p->_angularVelocity = generateScalar(_angularVelocityMin, _angularVelocityMax);
        p->_angle = generateScalar(0.0f, p->_angularVelocity);

        // Initial position, velocity and acceleration can be generated within ellipsoidal domains, or not.
        generateVector(_position, _positionVar, &p->_position, _ellipsoid);
        generateVector(_velocity, _velocityVar, &p->_velocity, false);
        generateVector(_acceleration, _accelerationVar, &p->_acceleration, false);

        // Initial position, velocity and acceleration can all be relative to the emitter's transform.
        // Rotate specified properties by the node's rotation.
        if (_orbitPosition)
        {
            world.transformPoint(p->_position, &p->_position);
        }

        if (_orbitVelocity)
        {
            world.transformPoint(p->_velocity, &p->_velocity);
        }

        if (_orbitAcceleration)
        {
            world.transformPoint(p->_acceleration, &p->_acceleration);
        }

        // Translate position relative to the node's world space.
        p->_position.add(translation);

        // Initial sprite frame.
        if (_spriteRandomOffset > 0)
        {
            p->_frame = rand() % _spriteRandomOffset;
        }
        else
        {
            p->_frame = 0;
        }
        p->_timeOnCurrentFrame = 0.0f;

        ++_particleCount;
    }
}

void ParticleEmitter::update(long elapsedTime)
{
    // Calculate the time passed since last update.
    //long time = Game::getGameTime();
    //long timeDelta = time - _timeLast;
    //float elapsedTime = (float)timeDelta / 1000.0f;
    //_timeLast = time;

    float elapsedSecs = (float)elapsedTime / 1000.0f;

    if (_active && _emissionRate)
    {
        // Calculate how much time has passed since we last emitted particles.
        _timeRunning += elapsedTime;

        // How many particles should we emit this frame?
        unsigned int emitCount = _timeRunning / _timePerEmission;
            
        _timeRunning %= _timePerEmission;

        emit(emitCount);
    }

    // Now update all currently living particles.
    for (unsigned int particlesIndex = 0; particlesIndex < _particleCount; ++particlesIndex)
    {
        Particle* p = &_particles[particlesIndex];
        p->_energy -= elapsedTime;

        if (p->_energy > 0L)
        {
            // Particle is still alive.
            p->_velocity.x += p->_acceleration.x * elapsedSecs;
            p->_velocity.y += p->_acceleration.y * elapsedSecs;
            p->_velocity.z += p->_acceleration.z * elapsedSecs;

            p->_position.x += p->_velocity.x * elapsedSecs;
            p->_position.y += p->_velocity.y * elapsedSecs;
            p->_position.z += p->_velocity.z * elapsedSecs;

            p->_angle += p->_angularVelocity * elapsedSecs;

            // Simple linear interpolation of color and size.
            float percent = 1.0f - ((float)p->_energy / (float)p->_energyStart);

            p->_color.r = p->_colorStart.r + (p->_colorEnd.r - p->_colorStart.r) * percent;
            p->_color.g = p->_colorStart.g + (p->_colorEnd.g - p->_colorStart.g) * percent;
            p->_color.b = p->_colorStart.b + (p->_colorEnd.b - p->_colorStart.b) * percent;
            p->_color.a = p->_colorStart.a + (p->_colorEnd.a - p->_colorStart.a) * percent;

            p->_size = p->_sizeStart + (p->_sizeEnd - p->_sizeStart) * percent;

            // Handle sprite animations.
            if (_spriteAnimating)
            {
                if (!_spriteLooped)
                {
                    // The last frame should finish exactly when the particle dies.
                    float percentSpent = 0.0f;
                    for (unsigned int i = 0; i < p->_frame; i++)
                    {
                        percentSpent += _spritePercentPerFrame;
                    }
                    p->_timeOnCurrentFrame = percent - percentSpent;
                    if (p->_frame < _spriteFrameCount - 1 &&
                        p->_timeOnCurrentFrame >= _spritePercentPerFrame)
                    {
                        ++p->_frame;
                    }
                    break;
                }
                else
                {
                    // _spriteFrameDurationSecs is an absolute time measured in seconds,
                    // and the animation repeats indefinitely.
                    p->_timeOnCurrentFrame += elapsedSecs;
                    if (p->_timeOnCurrentFrame >= _spriteFrameDurationSecs)
                    {
                        p->_timeOnCurrentFrame -= _spriteFrameDurationSecs;
                        ++p->_frame;
                        if (p->_frame == _spriteFrameCount)
                        {
                            p->_frame = 0;
                        }
                    }
                    break;
                }
            }
        }
        else
        {
            // Particle is dead.  Move the particle furthest from the start of the array
            // down to take its place, and re-use the slot at the end of the list of living particles.
            if (particlesIndex != _particleCount - 1)
            {
                _particles[particlesIndex] = _particles[_particleCount - 1];
            }
            --_particleCount;

            // TODO: if (_particleCount == 0 && _autoCleanup) { /* cleanup */ }
        }
    }
}

void ParticleEmitter::draw()
{
    if (_particleCount > 0)
    {
        _spriteBatch->begin();

        // Which draw call we use depends on whether particles are rotating.
        if (_angularVelocityMin == 0.0f && _angularVelocityMax == 0.0f)
        {
            // No rotation.
            for (unsigned int i = 0; i < _particleCount; i++)
            {
                Particle* p = &_particles[i];
                _spriteBatch->draw(p->_position.x, p->_position.y, p->_position.z,
                                   p->_size, p->_size,
                                   _texCoords[p->_frame * 4], _texCoords[p->_frame * 4 + 1], _texCoords[p->_frame * 4 + 2], _texCoords[p->_frame * 4 + 3],
                                   p->_color);
            }
        }
        else
        {
            // Rotation.
            Vector2 pivot(0.5f, 0.5f);

            for (unsigned int i = 0; i < _particleCount; i++)
            {
                Particle* p = &_particles[i];
                _spriteBatch->draw(p->_position, p->_size, p->_size,
                                   _texCoords[p->_frame * 4], _texCoords[p->_frame * 4 + 1], _texCoords[p->_frame * 4 + 2], _texCoords[p->_frame * 4 + 3],
                                   p->_color, pivot, p->_angle);
            }
        }

        // Disable writing to the depth buffer.
        GLboolean depthMask;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        glDepthMask(GL_FALSE);

        // Render.
        _spriteBatch->end();

        // Turn the depth mask back on if it was on before.
        glDepthMask(depthMask);
    }
}

}

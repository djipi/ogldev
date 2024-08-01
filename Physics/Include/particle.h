/*

        Copyright 2024 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#pragma once

#include <assert.h>

#include "ogldev_math_3d.h"

namespace OgldevPhysics
{

class Particle {

public:

    void SetPosition(const Vector3f& Position)
    {
        m_position = Position;
    }    


    void SetMass(float Mass)
    {
        assert(Mass > 0.0f);

        m_reciprocalMass = 1.0f / Mass;
    }


    void SetReciprocalMass(float ReciprocalMass)
    {
        m_reciprocalMass = ReciprocalMass;
    }


    void SetVelocity(const Vector3f& Velocity)
    {
        m_velocity = Velocity;
    }


    void SetAcceleration(const Vector3f& Acceleration)
    {
        m_acceleration = Acceleration;
    }


    void SetDamping(float Damping)
    {
        m_damping = Damping;
    }


    void Integrate(float dt);


    const Vector3f& GetPosition() const
    {
        return m_position;
    }

protected:

    Vector3f m_position = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f m_velocity = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f m_acceleration = Vector3f(0.0f, 0.0f, 0.0f);

    float m_damping = 0.999f;
    float m_reciprocalMass = 0.0f;
};

}
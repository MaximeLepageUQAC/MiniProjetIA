#include "Nade.h"
#include "../lua/Raven_Scriptor.h"
#include "misc/cgdi.h"
#include "../Raven_Bot.h"
#include "../Raven_Game.h"
#include "../constants.h"
#include "2d/WallIntersectionTests.h"
#include "../Raven_Map.h"

#include "../Raven_Messages.h"
#include "Messaging/MessageDispatcher.h"


//-------------------------- ctor ---------------------------------------------
//-----------------------------------------------------------------------------
Nade::Nade(Raven_Bot* shooter, Vector2D target) :

    Raven_Projectile(target,
        shooter->GetWorld(),
        shooter->ID(),
        shooter->Pos(),
        shooter->Facing(),
        script->GetInt("Nade_Damage"),
        script->GetDouble("Nade_Scale"),
        script->GetDouble("Nade_MaxSpeed"),
        script->GetDouble("Nade_Mass"),
        script->GetDouble("Nade_MaxForce")),

    m_dCurrentBlastRadius(0.0),
    m_dBlastRadius(script->GetDouble("Nade_BlastRadius"))
{
    assert(target != Vector2D());
}


//------------------------------ Update ---------------------------------------
//-----------------------------------------------------------------------------
void Nade::Update()
{
    if (!m_bImpacted)
    {
        m_vVelocity = MaxSpeed() * Heading();

        //make sure vehicle does not exceed maximum velocity
        m_vVelocity.Truncate(m_dMaxSpeed);

        //update the position
        m_vPosition += m_vVelocity;

        TestForImpact();
    }

    else
    {
        m_dCurrentBlastRadius += script->GetDouble("Nade_ExplosionDecayRate");

        //when the rendered blast circle becomes equal in size to the blast radius
        //the rocket can be removed from the game
        if (m_dCurrentBlastRadius > m_dBlastRadius)
        {
            m_bDead = true;
        }
    }
}

void Nade::TestForImpact()
{

    //if the projectile has reached the target position or it hits an entity
    //or wall it should explode/inflict damage/whatever and then mark itself
    //as dead


    //test to see if the line segment connecting the rocket's current position
    //and previous position intersects with any bots.
    Raven_Bot* hit = GetClosestIntersectingBot(m_vPosition - m_vVelocity, m_vPosition);

    //if hit
    if (hit)
    {
        m_bImpacted = true;

        //send a message to the bot to let it know it's been hit, and who the
        //shot came from
        Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
            m_iShooterID,
            hit->ID(),
            Msg_TakeThatMF,
            (void*)&m_iDamageInflicted);

        //test for bots within the blast radius and inflict damage
        InflictDamageOnBotsWithinBlastRadius();
    }

    //test for impact with a wall
    double dist;
    if (FindClosestPointOfIntersectionWithWalls(m_vPosition - m_vVelocity,
        m_vPosition,
        dist,
        m_vImpactPoint,
        m_pWorld->GetMap()->GetWalls()))
    {
        m_bImpacted = true;

        //test for bots within the blast radius and inflict damage
        InflictDamageOnBotsWithinBlastRadius();

        m_vPosition = m_vImpactPoint;

        return;
    }

    //test to see if rocket has reached target position. If so, test for
     //all bots in vicinity
    const double tolerance = 5.0;
    if (Vec2DDistanceSq(Pos(), m_vTarget) < tolerance * tolerance)
    {
        m_bImpacted = true;

        InflictDamageOnBotsWithinBlastRadius();
    }
}

//--------------- InflictDamageOnBotsWithinBlastRadius ------------------------
//
//  If the rocket has impacted we test all bots to see if they are within the 
//  blast radius and reduce their health accordingly
//-----------------------------------------------------------------------------
void Nade::InflictDamageOnBotsWithinBlastRadius()
{
    std::list<Raven_Bot*>::const_iterator curBot = m_pWorld->GetAllBots().begin();

    for (curBot; curBot != m_pWorld->GetAllBots().end(); ++curBot)
    {
        if (Vec2DDistance(Pos(), (*curBot)->Pos()) < m_dBlastRadius + (*curBot)->BRadius())
        {
            //send a message to the bot to let it know it's been hit, and who the
            //shot came from
            Dispatcher->DispatchMsg(SEND_MSG_IMMEDIATELY,
                m_iShooterID,
                (*curBot)->ID(),
                Msg_TakeThatMF,
                (void*)&m_iDamageInflicted);

        }
    }
}


//-------------------------- Render -------------------------------------------
//-----------------------------------------------------------------------------
void Nade::Render()
{

    gdi->GreenPen();
    gdi->WhiteBrush();
    gdi->Circle(Pos(), 4);

    if (m_bImpacted)
    {
        gdi->HollowBrush();
        gdi->Circle(Pos(), m_dCurrentBlastRadius);
    }
}
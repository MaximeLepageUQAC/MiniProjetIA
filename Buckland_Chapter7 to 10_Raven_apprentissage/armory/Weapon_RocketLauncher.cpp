#include "Weapon_RocketLauncher.h"
#include "../Raven_Bot.h"
#include "misc/Cgdi.h"
#include "../Raven_Game.h"
#include "../Raven_Map.h"
#include "../lua/Raven_Scriptor.h"
#include "fuzzy/FuzzyOperators.h"


//--------------------------- ctor --------------------------------------------
//-----------------------------------------------------------------------------
RocketLauncher::RocketLauncher(Raven_Bot*   owner):

                      Raven_Weapon(type_rocket_launcher,
                                   script->GetInt("RocketLauncher_DefaultRounds"),
                                   script->GetInt("RocketLauncher_MaxRoundsCarried"),
                                   script->GetDouble("RocketLauncher_FiringFreq"),
                                   script->GetDouble("RocketLauncher_IdealRange"),
                                   script->GetDouble("Rocket_MaxSpeed"),
                                   owner)
{
    //setup the vertex buffer
  const int NumWeaponVerts = 8;
  const Vector2D weapon[NumWeaponVerts] = {Vector2D(0, -3),
                                           Vector2D(6, -3),
                                           Vector2D(6, -1),
                                           Vector2D(15, -1),
                                           Vector2D(15, 1),
                                           Vector2D(6, 1),
                                           Vector2D(6, 3),
                                           Vector2D(0, 3)
                                           };
  for (int vtx=0; vtx<NumWeaponVerts; ++vtx)
  {
    m_vecWeaponVB.push_back(weapon[vtx]);
  }

  //setup the fuzzy module
  InitializeFuzzyModule();

}


//------------------------------ ShootAt --------------------------------------
//-----------------------------------------------------------------------------
inline void RocketLauncher::ShootAt(Vector2D pos)
{ 
  if (NumRoundsRemaining() > 0 && isReadyForNextShot())
  {
    //fire off a rocket!
    m_pOwner->GetWorld()->AddRocket(m_pOwner, pos);

    m_iNumRoundsLeft--;

    UpdateTimeWeaponIsNextAvailable();

    //add a trigger to the game so that the other bots can hear this shot
    //(provided they are within range)
    m_pOwner->GetWorld()->GetMap()->AddSoundTrigger(m_pOwner, script->GetDouble("RocketLauncher_SoundRange"));
  }
}

//---------------------------- Desirability -----------------------------------
//
//-----------------------------------------------------------------------------
double RocketLauncher::GetDesirability(double DistToTarget)
{
  if (m_iNumRoundsLeft == 0)
  {
    m_dLastDesirabilityScore = 0;
  }
  else
  {
    //fuzzify distance and amount of ammo
    m_FuzzyModule.Fuzzify("DistToTarget", DistToTarget);
    m_FuzzyModule.Fuzzify("AmmoStatus", (double)m_iNumRoundsLeft);

    m_dLastDesirabilityScore = m_FuzzyModule.DeFuzzify("Desirability", FuzzyModule::max_av);
  }

  return m_dLastDesirabilityScore;
}

//-------------------------  InitializeFuzzyModule ----------------------------
//
//  set up some fuzzy variables and rules
//-----------------------------------------------------------------------------
void RocketLauncher::InitializeFuzzyModule()
{
  FuzzyVariable& DistToTarget = m_FuzzyModule.CreateFLV("DistToTarget");

  FzSet& Target_VeryClose = DistToTarget.AddLeftShoulderSet("Target_VeryClose", 0, 5, 10);
  FzSet& Target_Close = DistToTarget.AddLeftShoulderSet("Target_Close",0,25,150);
  FzSet& Target_Medium = DistToTarget.AddTriangularSet("Target_Medium",25,150,300);
  FzSet& Target_Far = DistToTarget.AddRightShoulderSet("Target_Far",150,300,1000);
  FzSet& Target_VeryFar = DistToTarget.AddRightShoulderSet("Target_VeryFar", 300, 750, 1500);


  FuzzyVariable& Desirability = m_FuzzyModule.CreateFLV("Desirability"); 

  FzSet& VeryDesirable = Desirability.AddRightShoulderSet("VeryDesirable", 60, 75, 100);
  FzSet& PrettyDesirable = Desirability.AddTriangularSet("PrettyDesirable", 45, 60, 75);
  FzSet& Desirable = Desirability.AddTriangularSet("Desirable", 35, 45, 60);
  FzSet& NotVeryDesirable = Desirability.AddTriangularSet("NotVeryDesirable", 20, 35, 45);
  FzSet& Undesirable = Desirability.AddLeftShoulderSet("Undesirable", 0, 20, 35);

  FuzzyVariable& AmmoStatus = m_FuzzyModule.CreateFLV("AmmoStatus");

  FzSet& Ammo_Loads = AmmoStatus.AddRightShoulderSet("Ammo_Loads", 40, 80, 100);
  FzSet& Ammo_AlmostFull = AmmoStatus.AddRightShoulderSet("Ammo_AlmostFull", 20, 40, 80);
  FzSet& Ammo_Okay = AmmoStatus.AddTriangularSet("Ammo_Okay", 10, 20, 40);
  FzSet& Ammo_Low = AmmoStatus.AddTriangularSet("Ammo_Low", 5, 10, 20);
  FzSet& Ammo_AlmostEmpty = AmmoStatus.AddTriangularSet("Ammo_AlmostEmpty", 0, 0, 10);


  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Loads), NotVeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_AlmostFull), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Okay), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Low), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_AlmostEmpty), VeryDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_AlmostFull), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_AlmostEmpty), VeryDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_AlmostFull), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay), PrettyDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_AlmostEmpty), VeryDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_AlmostFull), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_AlmostEmpty), PrettyDesirable);

  m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_Loads), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_AlmostFull), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_Okay), NotVeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_Low), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryFar, Ammo_AlmostEmpty), PrettyDesirable);


}


//-------------------------------- Render -------------------------------------
//-----------------------------------------------------------------------------
void RocketLauncher::Render()
{
    m_vecWeaponVBTrans = WorldTransform(m_vecWeaponVB,
                                   m_pOwner->Pos(),
                                   m_pOwner->Facing(),
                                   m_pOwner->Facing().Perp(),
                                   m_pOwner->Scale());

  gdi->RedPen();

  gdi->ClosedShape(m_vecWeaponVBTrans);
}
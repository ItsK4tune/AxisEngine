

#ifndef BT_CONTACT_SOLVER_INFO
#define BT_CONTACT_SOLVER_INFO

#include "LinearMath/btScalar.h"

enum btSolverMode
{
	SOLVER_RANDMIZE_ORDER = 1,
	SOLVER_FRICTION_SEPARATE = 2,
	SOLVER_USE_WARMSTARTING = 4,
	SOLVER_USE_2_FRICTION_DIRECTIONS = 16,
	SOLVER_ENABLE_FRICTION_DIRECTION_CACHING = 32,
	SOLVER_DISABLE_VELOCITY_DEPENDENT_FRICTION_DIRECTION = 64,
	SOLVER_CACHE_FRIENDLY = 128,
	SOLVER_SIMD = 256,
	SOLVER_INTERLEAVE_CONTACT_AND_FRICTION_CONSTRAINTS = 512,
	SOLVER_ALLOW_ZERO_LENGTH_FRICTION_DIRECTIONS = 1024,
	SOLVER_DISABLE_IMPLICIT_CONE_FRICTION = 2048,
	SOLVER_USE_ARTICULATED_WARMSTARTING = 4096,
};

struct btContactSolverInfoData
{
	btScalar m_tau;
	btScalar m_damping;  
	btScalar m_friction;
	btScalar m_timeStep;
	btScalar m_restitution;
	int m_numIterations;
	btScalar m_maxErrorReduction;
	btScalar m_sor;          
	btScalar m_erp;          
	btScalar m_erp2;         
	btScalar m_deformable_erp;          
	btScalar m_deformable_cfm;          
	btScalar m_deformable_maxErrorReduction; 
	btScalar m_globalCfm;    
	btScalar m_frictionERP;  
	btScalar m_frictionCFM;  

	int m_splitImpulse;
	btScalar m_splitImpulsePenetrationThreshold;
	btScalar m_splitImpulseTurnErp;
	btScalar m_linearSlop;
	btScalar m_warmstartingFactor;
	btScalar m_articulatedWarmstartingFactor;
	int m_solverMode;
	int m_restingContactRestitutionThreshold;
	int m_minimumSolverBatchSize;
	btScalar m_maxGyroscopicForce;
	btScalar m_singleAxisRollingFrictionThreshold;
	btScalar m_leastSquaresResidualThreshold;
	btScalar m_restitutionVelocityThreshold;
	bool m_jointFeedbackInWorldSpace;
	bool m_jointFeedbackInJointFrame;
	int m_reportSolverAnalytics;
	int m_numNonContactInnerIterations;
};

struct btContactSolverInfo : public btContactSolverInfoData
{
	inline btContactSolverInfo()
	{
		m_tau = btScalar(0.6);
		m_damping = btScalar(1.0);
		m_friction = btScalar(0.3);
		m_timeStep = btScalar(1.f / 60.f);
		m_restitution = btScalar(0.);
		m_maxErrorReduction = btScalar(20.);
		m_numIterations = 10;
		m_erp = btScalar(0.2);
		m_erp2 = btScalar(0.2);
		m_deformable_erp = btScalar(0.06);
		m_deformable_cfm = btScalar(0.01);
		m_deformable_maxErrorReduction = btScalar(0.1);
		m_globalCfm = btScalar(0.);
		m_frictionERP = btScalar(0.2);  
		m_frictionCFM = btScalar(0.);
		m_sor = btScalar(1.);
		m_splitImpulse = true;
		m_splitImpulsePenetrationThreshold = -.04f;
		m_splitImpulseTurnErp = 0.1f;
		m_linearSlop = btScalar(0.0);
		m_warmstartingFactor = btScalar(0.85);
		m_articulatedWarmstartingFactor = btScalar(0.85);
		
		m_solverMode = SOLVER_USE_WARMSTARTING | SOLVER_SIMD;  
		m_restingContactRestitutionThreshold = 2;              
		m_minimumSolverBatchSize = 128;                        
		m_maxGyroscopicForce = 100.f;                          
		m_singleAxisRollingFrictionThreshold = 1e30f;          
		m_leastSquaresResidualThreshold = 0.f;
		m_restitutionVelocityThreshold = 0.2f;  
		m_jointFeedbackInWorldSpace = false;
		m_jointFeedbackInJointFrame = false;
		m_reportSolverAnalytics = 0;
		m_numNonContactInnerIterations = 1;   
	}
};


struct btContactSolverInfoDoubleData
{
	double m_tau;
	double m_damping;  
	double m_friction;
	double m_timeStep;
	double m_restitution;
	double m_maxErrorReduction;
	double m_sor;
	double m_erp;        
	double m_erp2;       
	double m_globalCfm;  
	double m_splitImpulsePenetrationThreshold;
	double m_splitImpulseTurnErp;
	double m_linearSlop;
	double m_warmstartingFactor;
	double m_articulatedWarmstartingFactor;
	double m_maxGyroscopicForce;  
	double m_singleAxisRollingFrictionThreshold;

	int m_numIterations;
	int m_solverMode;
	int m_restingContactRestitutionThreshold;
	int m_minimumSolverBatchSize;
	int m_splitImpulse;
	char m_padding[4];
};

struct btContactSolverInfoFloatData
{
	float m_tau;
	float m_damping;  
	float m_friction;
	float m_timeStep;

	float m_restitution;
	float m_maxErrorReduction;
	float m_sor;
	float m_erp;  

	float m_erp2;       
	float m_globalCfm;  
	float m_splitImpulsePenetrationThreshold;
	float m_splitImpulseTurnErp;

	float m_linearSlop;
	float m_warmstartingFactor;
	float m_articulatedWarmstartingFactor;
	float m_maxGyroscopicForce;

	float m_singleAxisRollingFrictionThreshold;
	int m_numIterations;
	int m_solverMode;
	int m_restingContactRestitutionThreshold;

	int m_minimumSolverBatchSize;
	int m_splitImpulse;
	
};

#endif  

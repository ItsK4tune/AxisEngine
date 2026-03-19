



#ifndef BT_MATERIAL_H
#define BT_MATERIAL_H


class btMaterial
{
	
public:
	btScalar m_friction;
	btScalar m_restitution;
	int pad[2];

	btMaterial() {}
	btMaterial(btScalar fric, btScalar rest)
	{
		m_friction = fric;
		m_restitution = rest;
	}
};

#endif  

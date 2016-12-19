#ifndef _Triangle_H
#define _Triangle_H

#include "math.h"
#include "Object.h"
#include "Vect.h"
#include "Color.h"

class Triangle : public Object {
	Vect A;
	Vect B;
	Vect C;
	Vect normal;
	double distance;
	Color color;
	
	public:
	
	Triangle ();
	
	Triangle (Vect, Vect, Vect, Color);
	
	Vect getTriangleNormal () 
	{
		Vect CA (C.getVectX() - A.getVectX(), C.getVectY() - A.getVectY(), C.getVectZ() - A.getVectZ());
		Vect BA (B.getVectX() - A.getVectX(), B.getVectY() - A.getVectY(), B.getVectZ() - A.getVectZ());
		normal = CA.crossProduct(BA).normalize();
		return normal;
	}
	double getTriangleDistance () 
	{ //How distance?
		normal = getTriangleNormal();
		distance = normal.dotProduct(A);
		return distance; 
	}
	Color getColor () { return color; }
	
	Vect getNormalAt(Vect point) 
	{
		normal = getTriangleNormal();
		return normal;
	}
	
	double findIntersection(Ray ray) 
	{
		Vect ray_direction = ray.getRayDirection();
		Vect rayOrigin = ray.getRayOrigin();

		normal = getTriangleNormal();
		distance = getTriangleDistance();

		double a = ray_direction.dotProduct(normal);
		
		if (a == 0) {
			// ray is parallel to the plane
			return -1;
		}
		else {
			double b = normal.dotProduct(ray.getRayOrigin().vectAdd(normal.vectMult(distance).negative()));
			double distanceToPlane = -1*b/a;

			double iXx = ray_direction.vectMult(distanceToPlane).getVectX() + rayOrigin.getVectX();
			double iXy = ray_direction.vectMult(distanceToPlane).getVectY() + rayOrigin.getVectY();
			double iXz = ray_direction.vectMult(distanceToPlane).getVectZ() + rayOrigin.getVectZ();

			Vect iX(iXx, iXy, iXz);

			Vect CA (C.getVectX() - A.getVectX(), C.getVectY() - A.getVectY(), C.getVectZ() - A.getVectZ());
			Vect iXA (iX.getVectX() - A.getVectX(), iX.getVectY() - A.getVectY(), iX.getVectZ() - A.getVectZ());
			double test1 = (CA.crossProduct(iXA)).dotProduct(normal);
			//CAxiXA(i POINX)
			Vect BC (B.getVectX() - C.getVectX(), B.getVectY() - C.getVectY(), B.getVectZ() - C.getVectZ());
			Vect iXC (iX.getVectX() - C.getVectX(), iX.getVectY() - C.getVectY(), iX.getVectZ() - C.getVectZ());
			double test2 = (BC.crossProduct(iXC)).dotProduct(normal);

			Vect AB (A.getVectX() - B.getVectX(), A.getVectY() - B.getVectY(), A.getVectZ() - B.getVectZ());
			Vect iXB (iX.getVectX() - B.getVectX(), iX.getVectY() - B.getVectY(), iX.getVectZ() - B.getVectZ());
			double test3 = (AB.crossProduct(iXC)).dotProduct(normal);
			if ((test3>=0) && (test2>=0) && (test3>=0))
			{
				return -1*b/a;
			}
			else
			{
				return -1;
			}
		}
	}
	
};

Triangle::Triangle () {
	A = Vect(1, 0, 0);
	B = Vect(0, 1, 0);
	C = Vect(0, 0, 1);
	color = Color(0.5,0.5,0.5, 0);
}

Triangle::Triangle (Vect pointA, Vect pointB, Vect pointC, Color colorValue) 
{
	A = pointA;
	B = pointB;
	C = pointC;
	color = colorValue;
}

#endif

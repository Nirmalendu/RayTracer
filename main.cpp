#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <limits>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "Vect.h"
#include "Ray.h"
#include "Camera.h"
#include "Color.h"
#include "Source.h"
#include "Light.h"
#include "Object.h"
#include "Sphere.h"
#include "Plane.h"
#include "Triangle.h"
#include <omp.h>

using namespace std;

struct RGBType {
	double r;
	double g;
	double b;
};

void savebmp (const char *filename, int w, int h, int dpi, RGBType *data) {
	FILE *outfile;
	int k = w*h;
	int s = 4*k;
	int filesize = 54 + s;
	
	double factor = 39.375;
	int m = static_cast<int>(factor);
	
	int ppm = dpi*m;
	
	unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0,0,0, 54,0,0,0};
	unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,24,0};
	
	bmpfileheader[ 2] = (unsigned char)(filesize);
	bmpfileheader[ 3] = (unsigned char)(filesize>>8);
	bmpfileheader[ 4] = (unsigned char)(filesize>>16);
	bmpfileheader[ 5] = (unsigned char)(filesize>>24);
	
	bmpinfoheader[ 4] = (unsigned char)(w);
	bmpinfoheader[ 5] = (unsigned char)(w>>8);
	bmpinfoheader[ 6] = (unsigned char)(w>>16);
	bmpinfoheader[ 7] = (unsigned char)(w>>24);
	
	bmpinfoheader[ 8] = (unsigned char)(h);
	bmpinfoheader[ 9] = (unsigned char)(h>>8);
	bmpinfoheader[10] = (unsigned char)(h>>16);
	bmpinfoheader[11] = (unsigned char)(h>>24);
	
	bmpinfoheader[21] = (unsigned char)(s);
	bmpinfoheader[22] = (unsigned char)(s>>8);
	bmpinfoheader[23] = (unsigned char)(s>>16);
	bmpinfoheader[24] = (unsigned char)(s>>24);
	
	bmpinfoheader[25] = (unsigned char)(ppm);
	bmpinfoheader[26] = (unsigned char)(ppm>>8);
	bmpinfoheader[27] = (unsigned char)(ppm>>16);
	bmpinfoheader[28] = (unsigned char)(ppm>>24);
	
	bmpinfoheader[29] = (unsigned char)(ppm);
	bmpinfoheader[30] = (unsigned char)(ppm>>8);
	bmpinfoheader[31] = (unsigned char)(ppm>>16);
	bmpinfoheader[32] = (unsigned char)(ppm>>24);
	
	outfile = fopen(filename,"wb");
	
	fwrite(bmpfileheader,1,14,outfile);
	fwrite(bmpinfoheader,1,40,outfile);
	
	for (int i = 0; i < k; i++) {
		//RGBType rgb = data[i];
		
		double red = (data[i].r)*255;
		double green = (data[i].g)*255;
		double blue = (data[i].b)*255;
		
		unsigned char color[3] = {(int)floor(blue),(int)floor(green),(int)floor(red)};
		
		fwrite(color,1,3,outfile);
	}
	
	fclose(outfile);
}

int winningObjectIndex(vector<double> objectIntersections) {
	// return the index of the winning intersection
	int minimumValueIndex;
	
	// prevent unnessary calculations
	if (objectIntersections.size() == 0) {
		// if there are no intersections
		return -1;
	}
	else if (objectIntersections.size() == 1) {
		if (objectIntersections.at(0) > 0) {
			// if that intersection is greater than zero then its our index of minimum value
			return 0;
		}
		else {
			// otherwise the only intersection value is negative
			return -1;
		}
	}
	else {
		// otherwise there is more than one intersection
		// first find the maximum value
		
		double max = 0;
		for (int i = 0; i < objectIntersections.size(); i++) {
			if (max < objectIntersections.at(i)) {
				max = objectIntersections.at(i);
			}
		}
		
		// then starting from the maximum value find the minimum positive value
		if (max > 0) {
			// we only want positive intersections
			for (int index = 0; index < objectIntersections.size(); index++) {
				if (objectIntersections.at(index) > 0 && objectIntersections.at(index) <= max) {
					max = objectIntersections.at(index);
					minimumValueIndex = index;
				}
			}
			
			return minimumValueIndex;
		}
		else {
			// all the intersections were negative
			return -1;
		}
	}
}

Color getColorAt(Vect intersectionPosition, Vect intersectingRayDirection, vector<Object*> sceneObjects, int closestObjectIndex, vector<Source*> lightSources, double intersectionOffset, double ambientLight) {
	
	Color closestObjectColor = sceneObjects.at(closestObjectIndex)->getColor();
	Vect closestObjectNormal = sceneObjects.at(closestObjectIndex)->getNormalAt(intersectionPosition);
	
	if (closestObjectColor.getMaterialProperty() == 2) {
		// checkered/tile floor pattern
		
		int checkered = (int)floor(intersectionPosition.getVectX()) + (int)floor(intersectionPosition.getVectZ());
		
		if ((checkered % 2) == 0) {
			// black tile
			closestObjectColor.setColorRed(0);
			closestObjectColor.setColorGreen(0);
			closestObjectColor.setColorBlue(0);
		}
		else {
			// white tile
			closestObjectColor.setColorRed(1);
			closestObjectColor.setColorGreen(1);
			closestObjectColor.setColorRed(1);
		}
	}
	
	Color finalColor = closestObjectColor.colorScalar(ambientLight);
	
	if (closestObjectColor.getMaterialProperty() > 0 && closestObjectColor.getMaterialProperty() <= 1) {
		// reflection from objects with specular intensity
		double dota = closestObjectNormal.dotProduct(intersectingRayDirection.negative());
		Vect scalara = closestObjectNormal.vectMult(dota);
		Vect adda = scalara.vectAdd(intersectingRayDirection);
		Vect scalarb = adda.vectMult(2);
		Vect addb = intersectingRayDirection.negative().vectAdd(scalarb);
		Vect reflectedDirection = addb.normalize();
		
		Ray reflectedRay (intersectionPosition, reflectedDirection);
		
		// determine what the ray intersects with first
		vector<double> reflectionIntersections;
		
		for (int reflection_index = 0; reflection_index < sceneObjects.size(); reflection_index++) {
			reflectionIntersections.push_back(sceneObjects.at(reflection_index)->findIntersection(reflectedRay));
		}
		
		int closestObjectIndexWithSpecular = winningObjectIndex(reflectionIntersections);
		
		if (closestObjectIndexWithSpecular != -1) {
			// reflection ray missed everthing else
			if (reflectionIntersections.at(closestObjectIndexWithSpecular) > intersectionOffset) {
				// determine the position and direction at the point of intersection with the reflection ray
				// the ray only affects the color if it reflected off something
				
				Vect reflectionIntersectionPosition = intersectionPosition.vectAdd(reflectedDirection.vectMult(reflectionIntersections.at(closestObjectIndexWithSpecular)));
				Vect reflectionIntersectionRayDirection = reflectedDirection;
				
				Color reflectionIntersectionColor = getColorAt(reflectionIntersectionPosition, reflectionIntersectionRayDirection, sceneObjects, closestObjectIndexWithSpecular, lightSources, intersectionOffset, ambientLight);
				
				finalColor = finalColor.colorAdd(reflectionIntersectionColor.colorScalar(closestObjectColor.getMaterialProperty()));
			}
		}
	}
	
	for (int lightIndex = 0; lightIndex < lightSources.size(); lightIndex++) {
		Vect lightDirection = lightSources.at(lightIndex)->getLightPosition().vectAdd(intersectionPosition.negative()).normalize();
		
		float cosineAngle = closestObjectNormal.dotProduct(lightDirection);
		
		if (cosineAngle > 0) {
			// test for shadows
			bool shadowed = false;
			
			Vect distanceToLight = lightSources.at(lightIndex)->getLightPosition().vectAdd(intersectionPosition.negative()).normalize();
			float distanceToLightMagnitude = distanceToLight.magnitude();
			
			Ray shadow_ray (intersectionPosition, lightSources.at(lightIndex)->getLightPosition().vectAdd(intersectionPosition.negative()).normalize());
			
			vector<double> secondaryIntersections;
			
			for (int object_index = 0; object_index < sceneObjects.size() && shadowed == false; object_index++) {
				secondaryIntersections.push_back(sceneObjects.at(object_index)->findIntersection(shadow_ray));
			}
			
			for (int c = 0; c < secondaryIntersections.size(); c++) {
				if (secondaryIntersections.at(c) > intersectionOffset) {
					if (secondaryIntersections.at(c) <= distanceToLightMagnitude) {
						shadowed = true;
					}
					break;
				}
				//break;
			}
			
			if (shadowed == false) {
				finalColor = finalColor.colorAdd(closestObjectColor.colorMultiply(lightSources.at(lightIndex)->getLightColor()).colorScalar(cosineAngle));
				
				if (closestObjectColor.getMaterialProperty() > 0 && closestObjectColor.getMaterialProperty() <= 1) {
					// special [0-1]
					double dota = closestObjectNormal.dotProduct(intersectingRayDirection.negative());
					Vect scalara = closestObjectNormal.vectMult(dota);
					Vect adda = scalara.vectAdd(intersectingRayDirection);
					Vect scalarb = adda.vectMult(2);
					Vect addb = intersectingRayDirection.negative().vectAdd(scalarb);
					Vect reflectedDirection = addb.normalize();
					
					double specular = reflectedDirection.dotProduct(lightDirection);
					if (specular > 0) {
						specular = pow(specular, 10);
						finalColor = finalColor.colorAdd(lightSources.at(lightIndex)->getLightColor().colorScalar(specular*closestObjectColor.getMaterialProperty()));
					}
				}
				
			}
			
		}
	}
	
	return finalColor.clip();
}



int main (int argc, char *argv[]) {
	cout << "rendering ..." << endl;
	int totalFrames = 4;
	clock_t starT, endT;
	starT = clock();
	omp_set_num_threads(4);
	int loop;
	#pragma omp parallel for private(loop)
	for (loop = 0; loop < totalFrames; loop++)
	{
		clock_t t1, t2;
		t1 = clock();

		int dpi = 72;
		int sceneX = 640;
		int sceneY = 480;
		int totalPixel = sceneX*sceneY;
		RGBType *pixels = new RGBType[totalPixel];

		int aaDepth = 1;
		double aaThreshold = 0.1;
		double aspectRatio = (double)sceneX/(double)sceneY;
		double ambientLight = 0.2;
		double intersectionOffset = 0.00000001;

		Vect O (0,0,0);
		Vect S3 (-2.2,1,0);
		Vect S4 (-0,2,0);
		Vect S5 (-0,3,0);
		Vect S6 (-0,1,0);
		Vect S7 (0,2,-0);
		Vect S8 (0,3,-0);
		Vect S9 (2,0,-0);
		Vect S10 (3,0,-2.0);
		Vect X (1,0,0);
		Vect Y (0,1,0);
		Vect Z (0,0,1);
		Vect tmpPos(0,0,2);
		float t = loop/(float)totalFrames;
		float startPointX = 1.3;
		float middlePoint1X = 1.3;
		float middlePoint2X = -1.3;
		float endPointX = -1.3;
		float xBZC = pow(1-t,2)*startPointX + 3*pow(1-t,2)*t*middlePoint1X + 3*(1-t)*t*t*middlePoint2X + pow(t, 3)*endPointX;
		float startPointZ = 0;
		float middlePoint1Z = -2.2;
		float middlePoint2Z = -2.2;
		float endPointZ = 0;
		float zBZC = pow(1-t,2)*startPointZ + 3*pow(1-t,2)*t*middlePoint1Z + 3*(1-t)*t*t*middlePoint2Z + pow(t, 3)*endPointZ;
		float startPointCx = -4.5;
		float endPointCx = 1.8;
		float startPointCz = -0.8;
		float endPointCz = -1.4;
		float middlePoint1Cx = -3.0;
		float middlePoint2Cx = 0.3;
		float middlePoint1Cz = -1.8;
		float middlePoint2Cz = 0.8;
		float cBZCx = pow(1-t,2)*startPointCx + 3*pow(1-t,2)*t*(middlePoint1Cx-0.0) + 3*(1-t)*t*t*(middlePoint2Cx-0.0) + pow(t, 3)*endPointCx;
		float cBZCz = pow(1-t,2)*startPointCz + 3*pow(1-t,2)*t*(middlePoint1Cz-0.0) + 3*(1-t)*t*t*(middlePoint2Cz-0.0) + pow(t, 3)*endPointCz;
		Vect sphereLocation (xBZC, 0, zBZC);

		Vect campos (cBZCx, 1.5, cBZCz);

		Vect lookAt (0, 0, 0);
		Vect dir1 (campos.getVectX() - lookAt.getVectX(), campos.getVectY() - lookAt.getVectY(), campos.getVectZ() - lookAt.getVectZ());

		Vect camDir = dir1.negative().normalize();
		Vect camRight = Y.crossProduct(camDir).normalize();
		Vect camDown = camRight.crossProduct(camDir);
		Camera sceneCam (campos, camDir, camRight, camDown);

		Color white (1.0, 1.0, 1.0, 0);
		Color blueL (0.2, 0.1, 1.0, 0.3);
		Color greenL (0.0, 51/255.0, 25/255.0, 0);
		Color purple (76/255.0, 0.0, 153.0/255, 0);
		Color brown (102.0/255, 51/255.0, 0.0, 0);
		Color Lblue (102/255.0, 1.0, 1.0, 0);
		Color yellow (1, 204/255.0, 204/255.0, 0.3);
		Color mirror (1.0, 1.0, 1.0, 0.6);
		Color greenSpecular (0.5, 1.0, 0.5, 0.3);
		Color maroonSpecular (0.5, 0.25, 0.25, 0.3);
		Color checkeredPattern (1, 1, 1, 2);
		Color gray (0.5, 0.5, 0.5, 0);
		Color black (0.0, 0.0, 0.0, 0);

		Vect lightPosition1 (-7,10,-10);
		Vect lightPosition2 (7,10,-5);

		Light sceneLight1 (lightPosition1, white);
		Light sceneLight2 (lightPosition2, brown);
		vector<Source*> lightSources;
		lightSources.push_back(dynamic_cast<Source*>(&sceneLight1));
		lightSources.push_back(dynamic_cast<Source*>(&sceneLight2));
		vector<Object*> sceneObjects;


	// scene objects
		Sphere sceneSphere1 (O, 0.4, greenSpecular);
		Sphere sceneSphere2 (sphereLocation, 0.4, maroonSpecular);
		Sphere sceneSphere3 (sphereLocation.negative(), 0.4, blueL);
		//Sphere sceneSphere4 (X.negative(), 0.4, purple);
		//Sphere sceneSphere5 (Z.negative(), 0.4, brown);
		Sphere sceneSphere6 (S3, 0.4, mirror);
		//Sphere sceneSphere7 (S8, 0.4, black);
		//Sphere sceneSphere8 (S4, 0.4, Lblue);

		Plane scenePlane1 (Y, -1, checkeredPattern);
		Vect vertex1 = Vect(-2,0,0);
		Vect vertex2 = Vect(0,2,0);
		Vect vertex3 = Vect(2,0,0);
		Vect vertex4 = Vect(-2,2.2,0);
		Vect vertex5 = Vect(0,2,0);
		Vect vertex6 = Vect(2,2.2,0);
		//Triangle sceneTriangle1 (vertex1, vertex2, vertex3 , mirror);
		//Triangle sceneTriangle2 (vertex4, vertex5, vertex6 , mirror);
		//Triangle sceneTriangle2 (Vect(0,2,0), Vect(2,0,0), Vect(2,2,0),   mirror);
		sceneObjects.push_back(dynamic_cast<Object*>(&sceneSphere1));
		sceneObjects.push_back(dynamic_cast<Object*>(&sceneSphere2));
		sceneObjects.push_back(dynamic_cast<Object*>(&sceneSphere3));
		//sceneObjects.push_back(dynamic_cast<Object*>(&sceneSphere4));
		//sceneObjects.push_back(dynamic_cast<Object*>(&sceneSphere5));
		sceneObjects.push_back(dynamic_cast<Object*>(&sceneSphere6));
		//sceneObjects.push_back(dynamic_cast<Object*>(&sceneSphere7));
		//sceneObjects.push_back(dynamic_cast<Object*>(&sceneSphere8));

		sceneObjects.push_back(dynamic_cast<Object*>(&scenePlane1));
		//sceneObjects.push_back(dynamic_cast<Object*>(&sceneTriangle1));
		//sceneObjects.push_back(dynamic_cast<Object*>(&sceneTriangle2));
		

		int frag, aaIndex;
		double xamnt, yamnt;
		double aaRed, aaGreen, aaBlue;

		for (int x = 0; x < sceneX; x++) {
			for (int y = 0; y < sceneY; y++) {
				frag = y*sceneX + x;

				double aaRed[aaDepth*aaDepth];
				double aaGreen[aaDepth*aaDepth];
				double aaBlue[aaDepth*aaDepth];

				for (int aax = 0; aax < aaDepth; aax++) {
					for (int aay = 0; aay < aaDepth; aay++) {

						aaIndex = aay*aaDepth + aax;


						if (aaDepth == 1) 
						{


							xamnt = ((x+0.5)/sceneX)*aspectRatio - (((sceneX-sceneY)/(double)sceneY)/2);
							yamnt = ((sceneY - y) + 0.5)/sceneY;
							
						}
						else 
						{
						// anti-aliasing
							xamnt = ((x + (double)aax/((double)aaDepth - 1))/sceneX)*aspectRatio - (((sceneX-sceneY)/(double)sceneY)/2);
							yamnt = ((sceneY - y) + (double)aax/((double)aaDepth - 1))/sceneY;
							
						}

						Vect camRayOrigin = sceneCam.getCameraPosition();
						Vect camRayDirection = camDir.vectAdd(camRight.vectMult(xamnt - 0.5).vectAdd(camDown.vectMult(yamnt - 0.5))).normalize();

						Ray camRay (camRayOrigin, camRayDirection);

						vector<double> intersections;

						for (int index = 0; index < sceneObjects.size(); index++) {
							intersections.push_back(sceneObjects.at(index)->findIntersection(camRay));
						}

						int closestObjectIndex = winningObjectIndex(intersections);

						if (closestObjectIndex == -1) {
						// ray missed
							aaRed[aaIndex] = 0;
							aaGreen[aaIndex] = 0;
							aaBlue[aaIndex] = 0;
						}
						else{
						// intersected
							if (intersections.at(closestObjectIndex) > intersectionOffset) {

								Vect intersectionPosition = camRayOrigin.vectAdd(camRayDirection.vectMult(intersections.at(closestObjectIndex)));
								Vect intersectingRayDirection = camRayDirection;

								Color intersection_color = getColorAt(intersectionPosition, intersectingRayDirection, sceneObjects, closestObjectIndex, lightSources, intersectionOffset, ambientLight);

								aaRed[aaIndex] = intersection_color.getColorRed();
								aaGreen[aaIndex] = intersection_color.getColorGreen();
								aaBlue[aaIndex] = intersection_color.getColorBlue();
							}
						}
					}
				}

			// supersampling
				double totalRed = 0;
				double totalGreen = 0;
				double totalBlue = 0;

				for (int indexR = 0; indexR < aaDepth*aaDepth; indexR++) {
					totalRed = totalRed + aaRed[indexR];
				}
				for (int indexG = 0; indexG < aaDepth*aaDepth; indexG++) {
					totalGreen = totalGreen + aaGreen[indexG];
				}
				for (int indexB = 0; indexB < aaDepth*aaDepth; indexB++) {
					totalBlue = totalBlue + aaBlue[indexB];
				}

				double avgRed = totalRed/(aaDepth*aaDepth);
				double avgGreen = totalGreen/(aaDepth*aaDepth);
				double avgBlue = totalBlue/(aaDepth*aaDepth);

				pixels[frag].r = avgRed;
				pixels[frag].g = avgGreen;
				pixels[frag].b = avgBlue;
			}
		}
		//const char* leName[5][10] =  {{"Name1.bmp"},{"Name2.bmp"},{"Name3.bmp"},{"Name4.bmp"},{"Name5.bmp"}};
		char files[8];
		snprintf(files, sizeof(files) * 32, "file%i.bmp", loop);
		savebmp(files,sceneX,sceneY,dpi,pixels);

		delete pixels, aaRed, aaGreen, aaBlue;

		t2 = clock();
		float diff = ((float)t2 - (float)t1)/1000;

		cout << diff << " seconds" << endl;
	}
	endT = clock();
	float df = ((float)endT - (float)starT)/1000;
	cout<<df;
	
	return 0;
}
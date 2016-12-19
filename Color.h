#ifndef _COLOR_H
#define _COLOR_H

class Color {
	double red, green, blue, property;
	
	public:
	
	Color ();
	
	Color (double, double, double, double);
	
	double getColorRed() { return red; }
	double getColorGreen() { return green; }
	double getColorBlue() { return blue; }
	double getMaterialProperty() { return property; }
	
	double setColorRed(double redValue) { red = redValue; }
	double setColorGreen(double greenValue) { green = greenValue; }
	double setColorBlue(double blueValue) { blue = blueValue; }
	double setMaterialProperty(double propertyValue) { property = propertyValue; }
	
	double brightness() {
		return(red + green + blue)/3;
	}
	
	Color colorScalar(double scalar) {
		return Color (red*scalar, green*scalar, blue*scalar, property);
	}
	
	Color colorAdd(Color color) {
		return Color (red + color.getColorRed(), green + color.getColorGreen(), blue + color.getColorBlue(), property);
	}
	
	Color colorMultiply(Color color) {
		return Color (red*color.getColorRed(), green*color.getColorGreen(), blue*color.getColorBlue(), property);
	}
	
	Color colorAverage(Color color) {
		return Color ((red + color.getColorRed())/2, (green + color.getColorGreen())/2, (blue + color.getColorBlue())/2, property);
	}
	
	Color clip() {
		double alllight = red + green + blue;
		double excesslight = alllight - 3;
		if (excesslight > 0) {
			red = red + excesslight*(red/alllight);
			green = green + excesslight*(green/alllight);
			blue = blue + excesslight*(blue/alllight);
		}
		if (red > 1) {red = 1;}
		if (green > 1) {green = 1;}
		if (blue > 1) {blue = 1;}
		if (red < 0) {red = 0;}
		if (green < 0) {green = 0;}
		if (blue < 0) {blue = 0;}
		
		return Color (red, green, blue, property);
	}
};

Color::Color () {
	red = 0.5;
	green = 0.5;
	blue = 0.5;
}

Color::Color (double r, double g, double b, double p) {
	red = r;
	green = g;
	blue = b;
	property = p;
}

#endif

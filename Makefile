OBJ = main.o
INC = -I "./"

Rasterrain: $(OBJ)
	g++ $(OBJ) -o RayTrEngine.exe -fopenmp
	del $(OBJ)
	
main.o:
	g++ -c main.cpp -fopenmp $(INC)
	
clean:
	del $(OBJ) Rasterrain
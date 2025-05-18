you need a sfml libriary --verison 2.6.2

1)configurate "c_cpp_properties.json" 

1.1)change the include path for library

            "includePath": [
                "${workspaceFolder}/**",
                "C:\\c++libraries\\SFML-2.6.2\\include" <-- here
            ],

1.2)Add/or chnage th compiler path

2) Compile the programm

2.1)g++ -c main.cpp -I"path to include folder(sfml)"

3)build .exe file

3.1) g++ main.o -o main -L"path to lib folder(sfml)" -lsfml-graphics -lsfml-window -lsfml-system

4)Run game.

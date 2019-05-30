#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>


struct tuple {
    std::string c;
    double x;
    double y;
    double z;
};


int main() {

    std::ifstream file("../bunny.obj");
    std::vector<tuple> vector_of_tuples;

    if (file.is_open()) {
        std::string line;

        while (file) {
            tuple temp;
            // std::stringstream ss(line);
            file >> temp.c >> temp.x >> temp.y >> temp.z;

            if (file) {
                vector_of_tuples.push_back(temp);
            }
            else if (!file.eof()) {
                file.clear();
                file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }

            file.close();

        }
    }


    for (int i = 0; i < vector_of_tuples.size(); i++) {
        tuple temp = vector_of_tuples[i];
        std::cout << temp.c << ", " << temp.x << ", " << temp.y << ", " << temp.z << std::endl;
    }

    return 0;
}

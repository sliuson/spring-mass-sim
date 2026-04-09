#include <SDL3/SDL.h>
#include <emscripten.h>
#include <vector>
#include <random>

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

class Spring;
class Mass;
float TIMESTEP = 1.0f/120.0f;

struct Vec {
    float x,y;
    Vec(float x, float y){
        this->x = x;
        this->y = y;
    }
    static Vec add(Vec a, Vec b){
        return Vec(a.x + b.x, a.y + b.y);
    }
    static Vec subtract(Vec a, Vec b){
        return Vec(a.x - b.x, a.y - b.y);
    }
    static Vec divide(Vec a, float scalar){
        return Vec(a.x / scalar, a.y / scalar);
    }
    static Vec multiply(Vec a, float scalar){
        return Vec(a.x * scalar, a.y * scalar);
    }
    static float getMagnitude(Vec a){
        return sqrt(a.x * a.x + a.y * a.y);
    }
};
class Mass {
    public:
    Vec pos;
    Vec lastPos;
    Vec accel;
    float m;

    Mass(Vec pos, float m) : pos(pos) ,m(m),lastPos(pos),accel(0,0){      
      
    }

    void display(){
        SDL_SetRenderDrawColor(renderer, 200,200,200,255);
        SDL_FRect rect = {this->pos.x-2.5f,this->pos.y-2.5f,5,5};
        SDL_RenderFillRect(renderer, &rect);
    }
    void applyForce(Vec force){
        accel = Vec::add(accel,Vec::divide(force,m));
    }
    void movement(){
        //verlet integration
        Vec tempCurrentPos = pos;
        pos = Vec::add(Vec::subtract(Vec::multiply(pos,2), lastPos), Vec::multiply(accel,TIMESTEP*TIMESTEP));
        lastPos = tempCurrentPos;
        accel = Vec(0,0);
    }
    

};
class Spring {
    public:
    float k;
    float equilibriumLength;
    Mass* m1;
    Mass* m2;
    Spring(Mass* m1, Mass* m2, float k, float equilibriumLength) : m1(m1),m2(m2),k(k),equilibriumLength(equilibriumLength){

    }
    void applyForce(){
        Vec displacement = Vec::subtract(m2->pos,m1->pos);
        float currentLength = Vec::getMagnitude(displacement);
        //negative extension means compression
        float extension = currentLength - equilibriumLength;
        float tension = k * extension;

        //get unit direction from m1 to m2
        Vec unitDirection = Vec::divide(displacement, Vec::getMagnitude(displacement));
        Vec forceOn1 = Vec::multiply(unitDirection, tension);
        Vec forceOn2 = Vec::multiply(Vec::multiply(unitDirection,-1), tension);

        m1->applyForce(forceOn1);
        m2->applyForce(forceOn2);

    }
    
    void display(){
        SDL_SetRenderDrawColor(renderer, 255,255,255,255);
        
        SDL_RenderLine(renderer, m1->pos.x,m1->pos.y,m2->pos.x,m2->pos.y);
    

    }
};


std::vector<Spring> springs;
std::vector<Mass> masses;
bool massesAreConnected(Mass* m1,Mass* m2){
    for (int i = 0; i < springs.size(); i ++){
        if (springs[i].m1 == m1 && springs[i].m2 == m2 ||
            springs[i].m2 == m1 && springs[i].m1 == m2){
            return true;
        }

    }
    return false;
}
void connectMasses(Mass* m1,Mass* m2){
    float displacement = Vec::getMagnitude(Vec::subtract(m1->pos,m2->pos));
    springs.emplace_back(m1,m2,100,displacement);
}



void initializeSystem() {
    int rows = 20;
    int cols = 40;
    int spacing = 17;
    int m = 10;
    int startTranslation = 100;
    masses.reserve(rows*cols);
    for (int row = 0; row < rows; row ++){
        for (int col = 0; col < cols; col++){
            masses.emplace_back(Vec(startTranslation+col*spacing,startTranslation+row*spacing),1);
            //when created, connect up,left,upleft
            //exceptions: on left, on top, on topleft
            int index = (row*cols) + col;
            int up        = index - cols;
            int upLeft   = index - cols -1;
            int upRight = index - cols + 1;
            int left      = index - 1;
            
            bool onTop = false;
            bool onRight = false;
            bool onLeft = false;
            if (index <= cols-1){
                onTop = true;
            }
            if (index % cols ==0){
                onLeft = true;
            }
            if (index % cols == cols-1){
                onRight = true;
            }
            
            //default behavior:
            if(!onTop && !onRight && !onLeft){
                connectMasses(&masses[index],&masses[up]);
                connectMasses(&masses[index],&masses[left]);
                connectMasses(&masses[index],&masses[upLeft]);
                connectMasses(&masses[index],&masses[upRight]);
            } else if (onTop){

                if(onLeft){
                    //do nothing
                }else{
                    connectMasses(&masses[index],&masses[left]);
                }
                
                
                
                //Right col, not top of column
            } else if (onRight){
                connectMasses(&masses[index],&masses[up]);
                connectMasses(&masses[index],&masses[left]);
                connectMasses(&masses[index],&masses[upLeft]);
                //left col, not top of col
            } else if (onLeft){
                connectMasses(&masses[index],&masses[up]);
                connectMasses(&masses[index],&masses[upRight]);
            }
        }
    }
    //pull bottom right a little to give the system some energy
    masses.back().pos.x+=2;
    masses.back().pos.y+=2;
    masses.back().lastPos = masses.back().pos;
    /*
    0,       1,     2,      3,   ...,   cols-1
    cols  cols+1  cols+2  cols+3, ...,  2cols-1
    2cols 2cols+1 2cols+2 2cols+3, ..., 3cols-1
    3cols 3cols+1 3cols+2 3cols+3, ..., 4cols-1
    ...
    (rows-1)(cols)                  ..., (rows)(cols)-1

    index = (row * cols) + col

    to get neighbors:
    right     = index +1;
    up right  = index - cols +1
    up        = index - cols
    up left   = index - cols -1
    left      = index - 1;
    down left = index + cols -1
    down      = index + cols
    down right= index + cols +1


    edge cases:
    no left: index % cols = 0
    no right: index % cols = cols-1
    no up: index <= cols-1
    no down: index >= (rows-1)(cols)

    if there's no up or no right, then there's no up-right
    */

}

Vec calculateMouseForce(Mass* m1){
    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX,&mouseY);
    Vec mousePos(mouseX,mouseY);
    Vec r = Vec::subtract(m1->pos,mousePos);
    float mouseConstant = 100000;
    float forceMag = mouseConstant* m1->m *1/(Vec::getMagnitude(r)*Vec::getMagnitude(r));
    Vec forceDirection = Vec::divide(r,Vec::getMagnitude(r));
    Vec force = Vec::multiply(forceDirection,forceMag);
    return force;
}

Uint64 lastTime = SDL_GetPerformanceCounter();
Uint64 freq = SDL_GetPerformanceFrequency(); 
float accum = 0.0f;
void mainloop(){

    Uint64 now = SDL_GetPerformanceCounter();
    //seconds since last loop
    float elapsed = (float)(now - lastTime)/(float)freq;
    lastTime = now;
    if (elapsed > 0.25f){
        elapsed = 0.25f;
    }
    accum+=elapsed;
    //every time we accumulate enough time, we update physics, keeping our timesteps constant
    while (accum >= TIMESTEP){
        for (int i = 0; i < springs.size(); i++){
        //adds to masses' acceleration vectors;
        springs[i].applyForce();
        }

        for (int i = 0; i < masses.size(); i ++){
            masses[i].applyForce(calculateMouseForce(&masses[i]));
            masses[i].movement();
        }
        accum -= TIMESTEP;
    }

    //background
    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
    SDL_RenderClear(renderer);
    

    for(int i = 0; i < springs.size(); i ++){
        springs[i].display();
    }
    for (int i = 0; i < masses.size(); i ++){
        masses[i].display();
    }
    

    SDL_RenderPresent(renderer);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer("sketch",1000,800,0,&window,&renderer);
    
    initializeSystem();
   
    emscripten_set_main_loop(mainloop,0,1);

}

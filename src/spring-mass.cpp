#include <SDL3/SDL.h>
#include <emscripten.h>
#include <vector>
#include <SDL3/SDL_video.h>

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

class Spring;
class Mass;
float TIMESTEP = 1.0f/120.0f;
float dampening = 0.99f;
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
    Vec accel;

    Vec vel;
    float m;

    Mass(Vec pos, float m) : pos(pos) ,m(m),accel(0,0),vel(0,0){      
      
    }

    void display(){
        SDL_SetRenderDrawColor(renderer, 200,200,200,255);
        SDL_FRect rect = {this->pos.x,this->pos.y,1,1};
        SDL_RenderFillRect(renderer, &rect);
    }
    void wallCollision(){
        int width;
        int height;
        SDL_GetWindowSize(window,&width,&height);
        if(pos.x > width){
            pos.x = width;
            vel.x = -0.02*vel.x;
        }
        if(pos.y > height){
            pos.y = height;
            vel.y = -0.02*vel.y;
        }
        if(pos.x <0){
            pos.x = 0;
            vel.x =-0.02*vel.x;
        }
        if(pos.y < 0){
            pos.y = 0;
            vel.y = -0.02*vel.y;
        }
    }
    void applyForce(Vec force){
        accel = Vec::add(accel,Vec::divide(force,m));
    }


    //velocity verlet:
    //1) x_(n+1) = x_n + v_n * dt + 0.5 * a_n * dt^2
    //2) v_(n+0.5) = v_n + 0.5*a_n*dt
    //3) a(t+dt) = f(x_n+1)
    //4) v(t+1)  = v_(n+0.5) + 0.5*a_(n+1)*dt
     void halfIncrementVelocity(){
        vel = Vec::add(vel,Vec::multiply(accel,0.5f*TIMESTEP));
    }
    void movement(){
        vel = Vec::multiply(vel,dampening);
        //1)apply velocity and acceleration          
        pos = Vec::add(Vec::add(pos,Vec::multiply(vel,TIMESTEP)),Vec::multiply(accel,TIMESTEP*TIMESTEP));

            //wall collison
        wallCollision();
        //2) calculate velocity half step forward
        vel = Vec::add(vel,Vec::multiply(accel,0.5f*TIMESTEP));
        //3) reset acceleration to prepare to calculate new acceleration
        accel = Vec(0,0);

     /* //  basic verlet
        Vec tempCurrentPos = pos;
        pos = Vec::add(Vec::subtract(Vec::multiply(pos,2), lastPos), Vec::multiply(accel,TIMESTEP*TIMESTEP));
        lastPos = tempCurrentPos;
        accel = Vec(0,0);
        */
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
        if(currentLength < 0.00001f){
            return;
        }
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
    springs.emplace_back(m1,m2,10000,displacement);
}



void initializeSystem() {
    int rows = 50;
    int cols = 50;
    int spacing = 9;
    int m = 10;
    int margin = 100;
    int width;
    int height;
    SDL_GetWindowSize(window,&width,&height);
    if(cols*spacing +2*margin>= width){
        cols = static_cast<int>((width - 2*margin)/spacing);
    }
    if(rows*spacing +2*margin>= height){
        rows = static_cast<int>((height - 2*margin)/spacing);
    }
    

    masses.reserve(rows*cols);
    for (int row = 0; row < rows; row ++){
        for (int col = 0; col < cols; col++){
            masses.emplace_back(Vec(margin+col*spacing,margin+row*spacing),m);
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
    float dist = Vec::getMagnitude(r);
    if (dist < 1 || dist > 100){
        return Vec(0,0);
    }
    float mouseConstant = 400000;
    float forceMag = mouseConstant*1/(dist);
    Vec forceDirection = Vec::divide(r,dist);
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
        //1) apply vel and accel
        //2) half update vel
        for (int i = 0; i < masses.size(); i ++){
            masses[i].movement();
        }

        //3) calculate new acceleration
        for (int i = 0; i < springs.size(); i++){
            springs[i].applyForce();
        }

        for (int i = 0; i < masses.size(); i ++){
            masses[i].applyForce(calculateMouseForce(&masses[i]));
        }

        //4) half increment velocity with new acceleration
        for (int i = 0; i < masses.size(); i ++){
            masses[i].halfIncrementVelocity();
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
    SDL_SetWindowFillDocument(window, true);
    initializeSystem();
   
    emscripten_set_main_loop(mainloop,0,1);

}

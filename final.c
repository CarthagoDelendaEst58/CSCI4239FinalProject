#include "CSCIx239.h"
#include "VBOOjbectData.h"
#include <strings.h>

float th=0,ph=0; //  View angles
int fov=57;    //  Field of view (for perspective)
float asp=1;   //  Aspect ratio
float dim=20;   //  Size of world
int shader[] = {0,0,0,0,0};  //  Shaders
const char * skybox_names[] = {"space/px.bmp","space/nx.bmp","space/py.bmp","space/ny.bmp","space/pz.bmp","space/nz.bmp"};
unsigned int cubemap=0;
unsigned char * data;
int accretion_disk = 0;
int max_iter = 1000;

int m1 = 0;
double cursor_xpos;
double cursor_ypos;

// double time;

int window_width=1500;
int window_height=1200;

unsigned int quad_vbo=0;
unsigned int quad_vao=0;
const int quad_size=6;
const float quad_data[] =  // Vertex data
{
   -1,-1,0,+1, 0,0,-1, 1,1,1,1, 0,0,
   -1,+1,0,+1, 0,0,-1, 1,1,1,1, 0,1,
   +1,+1,0,+1, 0,0,-1, 1,1,1,1, 1,1,
   
   +1,+1,0,+1, 0,0,-1, 1,1,1,1, 1,1,
   +1,-1,0,+1, 0,0,-1, 1,1,1,1, 1,0,
   -1,-1,0,+1, 0,0,-1, 1,1,1,1, 0,0,
};


// Pulled from loadtexbmp.c
static void Reverse(void* x,const int n)
{
   char* ch = (char*)x;
   for (int k=0;k<n/2;k++)
   {
      char tmp = ch[k];
      ch[k] = ch[n-1-k];
      ch[n-1-k] = tmp;
   }
}

// This function was ripped from LoadTexBMP in loadtexbmp.c
// I modified it to return the data without generating a texutre and to store the width and height of the image
unsigned char * loadData(const char * file, int * width, int * height) {
   //  Open file
   FILE* f = fopen(file,"rb");
   if (!f) Fatal("Cannot open file %s\n",file);
   //  Check image magic
   unsigned short magic;
   if (fread(&magic,2,1,f)!=1) Fatal("Cannot read magic from %s\n",file);
   if (magic!=0x4D42 && magic!=0x424D) Fatal("Image magic not BMP in %s\n",file);
   //  Read header
   unsigned int dx,dy,off,k; // Image dimensions, offset and compression
   unsigned short nbp,bpp;   // Planes and bits per pixel
   if (fseek(f,8,SEEK_CUR) || fread(&off,4,1,f)!=1 ||
       fseek(f,4,SEEK_CUR) || fread(&dx,4,1,f)!=1 || fread(&dy,4,1,f)!=1 ||
       fread(&nbp,2,1,f)!=1 || fread(&bpp,2,1,f)!=1 || fread(&k,4,1,f)!=1)
     Fatal("Cannot read header from %s\n",file);
   //  Reverse bytes on big endian hardware (detected by backwards magic)
   if (magic==0x424D)
   {
      Reverse(&off,4);
      Reverse(&dx,4);
      Reverse(&dy,4);
      Reverse(&nbp,2);
      Reverse(&bpp,2);
      Reverse(&k,4);
   }
   //  Check image parameters
   unsigned int max;
   glGetIntegerv(GL_MAX_TEXTURE_SIZE,(int*)&max);
   if (dx<1 || dx>max) Fatal("%s image width %d out of range 1-%d\n",file,dx,max);
   if (dy<1 || dy>max) Fatal("%s image height %d out of range 1-%d\n",file,dy,max);
   if (nbp!=1)  Fatal("%s bit planes is not 1: %d\n",file,nbp);
   if (bpp!=24) Fatal("%s bits per pixel is not 24: %d\n",file,bpp);
   if (k!=0)    Fatal("%s compressed files not supported\n",file);
#ifndef GL_VERSION_2_0
   //  OpenGL 2.0 lifts the restriction that texture size must be a power of two
   for (k=1;k<dx;k*=2);
   if (k!=dx) Fatal("%s image width not a power of two: %d\n",file,dx);
   for (k=1;k<dy;k*=2);
   if (k!=dy) Fatal("%s image height not a power of two: %d\n",file,dy);
#endif

   //  Allocate image memory
   unsigned int size = 3*dx*dy;
   unsigned char* image = (unsigned char*) malloc(size);
   if (!image) Fatal("Cannot allocate %d bytes of memory for image %s\n",size,file);
   //  Seek to and read image
   if (fseek(f,off,SEEK_SET) || fread(image,size,1,f)!=1) Fatal("Error reading data from image %s\n",file);
   fclose(f);
   //  Reverse colors (BGR -> RGB)
   for (k=0;k<size;k+=3)
   {
      unsigned char temp = image[k];
      image[k]   = image[k+2];
      image[k+2] = temp;
   }

   *width = dx;
   *height = dy;
   return image;
}

// This function was adapted into C from https://learnopengl.com/Advanced-OpenGL/Cubemaps
unsigned int loadCubemap(const char * names[6]) {
   unsigned int textureID;
   glEnable(GL_TEXTURE_CUBE_MAP);
   glGenTextures(1, &textureID);
   
   glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

   for (int i = 0; i < 6; i++) {
      int dx;
      int dy;
      data = loadData(names[i], &dx, &dy);
      if (data) {
         glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, dx, dy, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      }
      else {
         printf("data not loaded\n");
      }
      free(data);
   }
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

   return textureID;
}

void addSphereVertex(float * buf, float th, float ph, int * vert_num) {
   double x = Cos(th)*Cos(ph);
   double y = Sin(th)*Cos(ph);
   double z = Sin(ph);
   
   int offset = *vert_num*13;
   // Vertex
   buf[offset] = x;
   buf[offset+1] = y;
   buf[offset+2] = z;
   buf[offset+3] = 1;

   // Normal
   buf[offset+4] = x;
   buf[offset+5] = y;
   buf[offset+6] = z;

   // Color
   buf[offset+7] = 1;
   buf[offset+8] = 1;
   buf[offset+9] = 1;
   buf[offset+10] = 1;

   // Texture Coordinates
   buf[offset+11] = th/360;
   buf[offset+12] = ph/180+0.5;

   *vert_num += 1;
}

// Exports data
void exportSphereData(float * buf, int vert_num, char * filename) {
   int num_len = 32;
   int floats_per_line = 13;
   
   FILE * fp = fopen(filename, "w");
   char num[num_len];
   char to_write[1000000];
   memset(to_write, 0, 1000000);
   int i;
   for (i = 0; i < vert_num*floats_per_line; i++) {
      memset(num, 0, num_len);
      sprintf(num, "%f,", buf[i]);
      strcat(to_write, num);
      if (((i+1)%(floats_per_line)) == 0 && i) {
         strcat(to_write, "\n");
      }
   }
   fputs(to_write, fp);
   fclose(fp);
}

// Generates data resembling a sphere made of triangles
// This was adapted from SolidSphere in sphere.c
float * generateSphere(int n, int * vert_num) {
   *vert_num = 0;
   float * buf = malloc(6*n*n*sizeof(float)*13);
   for (int i=0;i<n;i++)
   {
      float ph0 =   i  *180.0/n-90;
      float ph1 = (i+1)*180.0/n-90;
      for (int j=0;j<n;j++)
      {
         float th0 = j*360.0/n;
         float th1 = (j+1)*360.0/n;

         addSphereVertex(buf, th0, ph0, vert_num);
         addSphereVertex(buf, th0, ph1, vert_num);
         addSphereVertex(buf, th1, ph1, vert_num);
         addSphereVertex(buf, th1, ph1, vert_num);
         addSphereVertex(buf, th0, ph0, vert_num);
         addSphereVertex(buf, th1, ph0, vert_num);
      }
   }
   printf("vert_num: %d\n", *vert_num);
   exportSphereData(buf, *vert_num, "sphere_data.txt");
   // return vert_num;
   return buf;
}

float * loadVBOData(char * filename, int num_vertices) {
   int line_size = 1024;
   char line[line_size];
   memset(line, 0, line_size);
   FILE * fp;
   float * obj_data = malloc(num_vertices*13*sizeof(float));
   int i = 0;
   int line_num = 0;
   if ((fp = fopen(filename, "r"))) {
      while (fgets(line, line_size, fp)) {
         char * token = strtok(line, ",");
         obj_data[i] = atof(token);
         i++;
         // printf("%f\n", atof(token));
         int hold = i;
         for (i = hold; i < hold+12; i++) {
            token = strtok(NULL, ",");
            obj_data[i] = atof(token);
         }
         memset(line, 0, line_size);
         line_num++;
      }
   }
   else {
      return NULL;
   }
   return obj_data;
}


// Initializes a VBO
void InitVBO(unsigned int * vbo, int size, const float data[])
{
   //  Get buffer name
   glGenBuffers(1,vbo);
   //  Bind VBO
   glBindBuffer(GL_ARRAY_BUFFER,*vbo);
   //  Copy cube to VBO
   glBufferData(GL_ARRAY_BUFFER,size*13*sizeof(float),data,GL_STATIC_DRAW);
   //  Release VBO
   glBindBuffer(GL_ARRAY_BUFFER,0);
}

// Initializes a VAO
void InitVAO(int shader, unsigned int * vao, unsigned int * vbo)
{
   //  Shader for which to get attibute locations
   glUseProgram(shader);

   //  Create cube VAO to bind attribute arrays
   glGenVertexArrays(1,vao);
   glBindVertexArray(*vao);

   ErrCheck("Post Bind Vertex array");

   //  Bind VBO
   glBindBuffer(GL_ARRAY_BUFFER,*vbo);
   //  Vertex
   int loc = glGetAttribLocation(shader,"Vertex");
   if (loc >= 0) {
      glVertexAttribPointer(loc,4,GL_FLOAT,0,52,(void*) 0);
      glEnableVertexAttribArray(loc);
   }
   //  Normal
   loc = glGetAttribLocation(shader,"Normal");
   if (loc >= 0) {
      glVertexAttribPointer(loc,3,GL_FLOAT,0,52,(void*)16);
      glEnableVertexAttribArray(loc);
   }
   ErrCheck("pre color attribute location");
   //  Color
   loc  = glGetAttribLocation(shader,"Color");
   if (loc >= 0) {
      glVertexAttribPointer(loc,4,GL_FLOAT,0,52,(void*)28);
      glEnableVertexAttribArray(loc);
   }
   

   ErrCheck("pre texture attribute location");

   //  Texture
   loc  = glGetAttribLocation(shader,"Texture");
   if (loc >= 0) {
      glVertexAttribPointer(loc,2,GL_FLOAT,0,52,(void*)44);
      glEnableVertexAttribArray(loc);
   }

   ErrCheck("Post vertex attribute pointers");

   //  Release VBO
   glBindBuffer(GL_ARRAY_BUFFER,0);
   //  Release VAO
   glBindVertexArray(0);
   //  Release shader
   glUseProgram(0);
}

// Creates the various matrices depending on the parameters
void setMatrices(int shader, float x, float y, float z, float dx, float dy, float dz, float th, float ph, float Ex, float Ey, float Ez, float fov) {
   float proj[16];
   mat4identity(proj);
   if (fov)
      mat4perspective(proj , fov,asp,dim/16,16*dim);
   else
      mat4ortho(proj , -dim*asp, +dim*asp, -dim, +dim, -dim, +dim);
   //  Create View matrix
   float view[16];
   mat4identity(view);
   if (fov)
      // mat4lookAt(view , Ex,Ey,Ez , 0,0,0 , 0,Cos(ph),0);
      mat4lookAt(view , 0,0,-dim , 0,0,0 , 0,Cos(ph),0);
   else
   {
      mat4rotate(view , ph,1,0,0);
      mat4rotate(view , th,0,1,0);
   }
   //  Create ModelView matrix
   float modelview[16];
   mat4copy(modelview , view);
   mat4translate(modelview , x,y,z);
   mat4scale(modelview, dx, dy, dz);

   mat4lookAt(view , Ex,Ey,Ez , 0,0,0 , 0,Cos(ph),0);

   //  Create Normal matrix
   float normat[9];
   mat4normalMatrix(modelview , normat);

   int id = glGetUniformLocation(shader,"ProjectionMatrix");
   glUniformMatrix4fv(id,1,0,proj);
   id = glGetUniformLocation(shader,"ViewMatrix");
   glUniformMatrix4fv(id,1,0,view);
   id = glGetUniformLocation(shader,"ModelViewMatrix");
   glUniformMatrix4fv(id,1,0,modelview);
   id = glGetUniformLocation(shader,"NormalMatrix");
   glUniformMatrix3fv(id,1,0,normat);
}

//
//  Refresh display
//
void display(GLFWwindow* window)
{
   // int id;
   //  Erase the window and the depth buffer
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   //  Enable Z-buffering in OpenGL
   glEnable(GL_DEPTH_TEST);

   glUseProgram(0);

   //  Eye position for perspective
   float Ex = -2*dim/2*Sin(th)*Cos(ph);
   float Ey = +2*dim/2        *Sin(ph);
   float Ez = +2*dim/2*Cos(th)*Cos(ph);

   glUseProgram(shader[4]);

   glBindVertexArray(quad_vao);
   
   setMatrices(shader[4], 0,0,0,30,15,15,th,ph, Ex, Ey, Ez, fov);
   glUniform1i(glGetUniformLocation(shader[4], "cubemap"), 0);
   glUniform3f(glGetUniformLocation(shader[4], "eyepos"), Ex, Ey, Ez);
   glUniform3f(glGetUniformLocation(shader[4], "center"), 0, 0, 0);
   glUniform1f(glGetUniformLocation(shader[4], "horizon_radius"), 1);
   glUniform1f(glGetUniformLocation(shader[4], "asp"), asp);
   glUniform2f(glGetUniformLocation(shader[4], "resolution"), window_width, window_height);
   glUniform1i(glGetUniformLocation(shader[4], "accretion_disk"), accretion_disk);
   glUniform1i(glGetUniformLocation(shader[4], "max_iter"), max_iter);

   glDrawArrays(GL_TRIANGLES,0,quad_size);
   
   glBindVertexArray(0);
   
   glUseProgram(0);

   SetColor(1,1,1);
   glWindowPos2i(5,5);
   Print("Steps: %d", max_iter);

   ErrCheck("display");
   glFlush();
   glfwSwapBuffers(window);
}

// Prevent camera from being upside down
void bindAngles(float* ph) {
   // printf("ph:%f\n", *ph);
   if (*ph > 90) {
      *ph = 90;
   }
   else if (*ph < -90) {
      *ph = -90;
   }
}

//
//  Key pressed callback
//
void key(GLFWwindow* window,int key,int scancode,int action,int mods)
{
   //  Discard key releases (keeps PRESS and REPEAT)
   if (action==GLFW_RELEASE) return;

   //  Exit on ESC
   if (key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(window,1);
   //  Reset view angle and location
   else if (key==GLFW_KEY_0)
      th = ph = 0;
   else if (key==GLFW_KEY_F) {
      // printf("%d\n", accretion_disk+1 );
      accretion_disk = (accretion_disk+1) % 2;
      
   }
   //  Increase/decrease asimuth
   else if (key==GLFW_KEY_RIGHT)
      th += 5;
   else if (key==GLFW_KEY_LEFT)
      th -= 5;
   //  Increase/decrease elevation
   else if (key==GLFW_KEY_UP)
      ph += 5;
   else if (key==GLFW_KEY_DOWN)
      ph -= 5;
   else if (key==GLFW_KEY_S)
      max_iter -= 5;
   else if (key==GLFW_KEY_W)
      max_iter += 5;
   //  Wrap angles
   th = fmod(th, 360);
   ph = fmod(ph, 360);
   bindAngles(&ph);
}

// Sets m1 depending ont the mouse state
void mouse(GLFWwindow* window, int button, int action, int mods) {
   if (button == GLFW_MOUSE_BUTTON_LEFT) {
      if (action == GLFW_PRESS) {
         m1 = 1;
      }
      else {
         m1 = 0;
      }
   }
}

// Changes view according to cursor movement while m1 is held down
void cursor(GLFWwindow* window, double xpos, double ypos) {
   if (m1) {
      double xdiff = cursor_xpos - xpos;
      double ydiff = cursor_ypos - ypos;
      th -= xdiff/10;
      ph += ydiff/10;
      bindAngles(&ph);
   }
   cursor_xpos = xpos;
   cursor_ypos = ypos;
}

//
//  Window resized callback
//
void reshape(GLFWwindow* window,int width,int height)
{
   //  Get framebuffer dimensions (makes Apple work right)
   glfwGetFramebufferSize(window,&width,&height);
   //  Ratio of the width to the height of the window
   asp = (height>0) ? (double)width/height : 1;
   //  Set the viewport to the entire window
   glViewport(0,0, width,height);

   window_width = width;
   window_height = height;
}

//
//  Main program with GLFW event loop
//
int main(int argc,char* argv[])
{
   //  Initialize GLFW
   GLFWwindow* window = InitWindow("Robert Dumitrescu - Final Project",1,window_width,window_height,&reshape,&key);
   glfwSetMouseButtonCallback(window, mouse);
   glfwSetCursorPosCallback(window, cursor);

   // Create shader
   shader[4] = CreateShaderProg("scene.vert", "scene.frag");

   ErrCheck("pre VBO, VAO");

   // Load quad into vbo
   InitVBO(&quad_vbo, quad_size, quad_data);
   

   ErrCheck("VBO initialized");

   // Create quad vao
   InitVAO(shader[4], &quad_vao, &quad_vbo);


   ErrCheck("pre loadCubemap");

   // Load skybox texture
   glActiveTexture(GL_TEXTURE0);
   cubemap = loadCubemap(skybox_names);


   //  Event loop
   ErrCheck("init");
   while(!glfwWindowShouldClose(window))
   {
      //  Display
      display(window);
      //  Process any events
      glfwPollEvents();
   }
   //  Shut down GLFW
   glfwDestroyWindow(window);
   glfwTerminate();

   return 0;
}

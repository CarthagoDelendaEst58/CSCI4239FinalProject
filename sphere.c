//  CSCIx239 library
//  Willem A. (Vlakkies) Schreuder
#include "CSCIx239.h"

//
//  Vertex in polar coordinates
//
static void Vertex(double th,double ph)
{
   double x = Cos(th)*Cos(ph);
   double y = Sin(th)*Cos(ph);
   double z =         Sin(ph);
   glNormal3d(x,y,z);
   glTexCoord2d(th/360,ph/180+0.5);
   glVertex3d(x,y,z);
}

//
//  Solid unit sphere
//
void SolidSphere(int n)
{
   //  Latitude bands
   for (int i=0;i<n;i++)
   {
      float ph0 =   i  *180.0/n-90;
      float ph1 = (i+1)*180.0/n-90;
      glBegin(GL_QUAD_STRIP);
      for (int j=0;j<=n;j++)
      {
         float th = j*360.0/n;
         Vertex(th,ph0);
         Vertex(th,ph1);
      }
      glEnd();
   }
}

void SolidSphereTriangles(int n) {
   glBegin(GL_TRIANGLES);
   for (int i=0;i<n;i++)
   {
      float ph0 =   i  *180.0/n-90;
      float ph1 = (i+1)*180.0/n-90;
      for (int j=0;j<n;j++)
      {
         float th0 = j*360.0/n;
         float th1 = (j+1)*360.0/n;

         // printf("ph0:%f, ph1:%f, th0:%f, th1:%f\n", ph0,ph1,th0,th1);
         // addSphereVertex(buf, th0, ph0, vert_num);
         // addSphereVertex(buf, th0, ph1, vert_num);
         // addSphereVertex(buf, th1, ph1, vert_num);
         // addSphereVertex(buf, th1, ph1, vert_num);
         // addSphereVertex(buf, th0, ph0, vert_num);
         // addSphereVertex(buf, th1, ph0, vert_num);
         Vertex(th0, ph0);
         Vertex(th0, ph1);
         Vertex(th1, ph1);
         Vertex(th1, ph1);
         Vertex(th0, ph0);
         Vertex(th1, ph0);

      }
   }
   glEnd();
}


//
//  Textured unit sphere
//
void TexturedSphere(int n,int tex)
{
   if (tex)
   {
      //  Enable texture
      glBindTexture(GL_TEXTURE_2D,tex);
      glEnable(GL_TEXTURE_2D);
      //  Draw sphere
      SolidSphere(n);
      // Restore
      glDisable(GL_TEXTURE_2D);
   }
   else
      SolidSphere(n);
}

//
//  General sphere
//
void Sphere(float x,float y,float z,float r,float th,int n,int tex)
{
   //  Scale
   glPushMatrix();
   glTranslatef(x,y,z);
   glScalef(r,r,r);
   glRotatef(th,0,1,0);
   //  Draw textured sphere
   TexturedSphere(n,tex);
   //  Undo transformations
   glPopMatrix();
}

#include "libs.h"
#include "Planet.h"
#include "Frame.h"
#include "Pi.h"
#include "WorldView.h"
#include "Serializer.h"
#include "StarSystem.h"
#include "perlin.h"

Planet::Planet(SBody *sbody): Body()
{
	pos = vector3d(0,0,0);
	this->sbody = sbody;
	this->m_geosphere = 0;
	Init();
}

void Planet::Init()
{
	m_mass = sbody->GetMass();
	crudDList = 0;
}
	
void Planet::Save()
{
	using namespace Serializer::Write;
	Body::Save();
	wr_vector3d(pos);
	wr_int(Serializer::LookupSystemBody(sbody));
}

void Planet::Load()
{
	using namespace Serializer::Read;
	Body::Load();
	pos = rd_vector3d();
	sbody = Serializer::LookupSystemBody(rd_int());
	Init();
}

Planet::~Planet()
{
}

double Planet::GetRadius() const
{
	return sbody->GetRadius();
}

vector3d Planet::GetPosition() const
{
	return pos;
}

void Planet::SetPosition(vector3d p)
{
	pos = p;
}

void Planet::SetRadius(double radius)
{
	assert(0);
}

static void subdivide(vector3d &v1, vector3d &v2, vector3d &v3, vector3d &v4, int depth)
{
	if (depth) {
		depth--;
		vector3d v5 = (v1+v2).Normalized();
		vector3d v6 = (v2+v3).Normalized();
		vector3d v7 = (v3+v4).Normalized();
		vector3d v8 = (v4+v1).Normalized();
		vector3d v9 = (v1+v2+v3+v4).Normalized();

		subdivide(v1,v5,v9,v8,depth);
		subdivide(v5,v2,v6,v9,depth);
		subdivide(v9,v6,v3,v7,depth);
		subdivide(v8,v9,v7,v4,depth);
	} else {
		glBegin(GL_TRIANGLE_STRIP);
		glNormal3dv(&v1.x);
		glVertex3dv(&v1.x);
		glNormal3dv(&v2.x);
		glVertex3dv(&v2.x);
		glNormal3dv(&v4.x);
		glVertex3dv(&v4.x);
		glNormal3dv(&v3.x);
		glVertex3dv(&v3.x);
		glEnd();
	}
}

static void DrawShittyRoundCube(double radius)
{
	vector3d p1(1,1,1);
	vector3d p2(-1,1,1);
	vector3d p3(-1,-1,1);
	vector3d p4(1,-1,1);

	vector3d p5(1,1,-1);
	vector3d p6(-1,1,-1);
	vector3d p7(-1,-1,-1);
	vector3d p8(1,-1,-1);

	p1 = p1.Normalized();
	p2 = p2.Normalized();
	p3 = p3.Normalized();
	p4 = p4.Normalized();
	p5 = p5.Normalized();
	p6 = p6.Normalized();
	p7 = p7.Normalized();
	p8 = p8.Normalized();

//	p1 *= radius;
//	p2 *= radius;
//	p3 *= radius;
//	p4 *= radius;
//	p5 *= radius;
//	p6 *= radius;
//	p7 *= radius;
//	p8 *= radius;

//	glDisable(GL_CULL_FACE);
	glEnable(GL_NORMALIZE);
	subdivide(p1, p2, p3, p4, 4);
	subdivide(p4, p3, p7, p8, 4);
	subdivide(p1, p4, p8, p5, 4);
	subdivide(p2, p1, p5, p6, 4);
	subdivide(p3, p2, p6, p7, 4);
	subdivide(p8, p7, p6, p5, 4);
	
	glDisable(GL_NORMALIZE);
}

// both arguments in radians
void DrawHoop(float latitude, float width, float wobble, MTRand &rng)
{
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);
	
	glBegin(GL_TRIANGLE_STRIP);
	for (double longitude=0.0f; longitude < 2*M_PI; longitude += 0.02) {
		vector3d v;
		double l;
		l = latitude+0.5*width+rng.Double(wobble*width);
		v.x = sin(longitude)*cos(l);
		v.y = sin(l);
		v.z = cos(longitude)*cos(l);
		v = v.Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
		
		l = latitude-0.5*width-rng.Double(wobble*width);
		v.x = sin(longitude)*cos(l);
		v.y = sin(l);
		v.z = cos(longitude)*cos(l);
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
	}
	double l = latitude+0.5*width;
	vector3d v;
	v.x = 0; v.y = sin(l); v.z = cos(l);
	v = v.Normalized();
	glNormal3dv(&v.x);
	glVertex3dv(&v.x);
	
	l = latitude-0.5*width;
	v.x = 0; v.y = sin(l); v.z = cos(l);
	glNormal3dv(&v.x);
	glVertex3dv(&v.x);

	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

static void PutPolarPoint(float latitude, float longitude)
{
	vector3d v;
	v.x = sin(longitude)*cos(latitude);
	v.y = sin(latitude);
	v.z = cos(longitude)*cos(latitude);
	v = v.Normalized();
	glNormal3dv(&v.x);
	glVertex3dv(&v.x);
}

void DrawBlob(float latitude, float longitude, float a, float b)
{
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);

	glBegin(GL_TRIANGLE_FAN);
	PutPolarPoint(latitude, longitude);
	for (double theta=2*M_PI; theta>0; theta-=0.1) {
		double _lat = latitude + a * cos(theta);
		double _long = longitude + b * sin(theta);
		PutPolarPoint(_lat, _long);
	}
	{
		double _lat = latitude + a;
		double _long = longitude;
		PutPolarPoint(_lat, _long);
	}
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

static void DrawRing(double inner, double outer, const float color[4])
{
	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
		GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glDisable(GL_CULL_FACE);

	glColor4fv(color);
	
	glBegin(GL_TRIANGLE_STRIP);
	glNormal3f(0,1,0);
	for (float ang=0; ang<2*M_PI; ang+=0.1) {
		glVertex3f(inner*sin(ang), 0, inner*cos(ang));
		glVertex3f(outer*sin(ang), 0, outer*cos(ang));
	}
	glVertex3f(0, 0, inner);
	glVertex3f(0, 0, outer);
	glEnd();

	//gluDisk(Pi::gluQuadric, inner, outer, 40, 20);
	glEnable(GL_CULL_FACE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

static void SphereTriSubdivide(vector3d &v1, vector3d &v2, vector3d &v3, int depth)
{
	if (--depth > 0) {
		vector3d v4 = (v1+v2).Normalized();
		vector3d v5 = (v2+v3).Normalized();
		vector3d v6 = (v1+v3).Normalized();
		SphereTriSubdivide(v1,v4,v6,depth);
		SphereTriSubdivide(v4,v2,v5,depth);
		SphereTriSubdivide(v6,v4,v5,depth);
		SphereTriSubdivide(v6,v5,v3,depth);
	} else {
		glNormal3dv(&v1.x);
		glVertex3dv(&v1.x);
		glNormal3dv(&v2.x);
		glVertex3dv(&v2.x);
		glNormal3dv(&v3.x);
		glVertex3dv(&v3.x);
	}
}

// yPos should be 1.0 for north pole, -1.0 for south pole
// size in radians
static void DrawPole(double yPos, double size)
{
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);

	const bool southPole = yPos < 0;
	size = size*4/M_PI;
	
	vector3d center(0, yPos, 0);
	glBegin(GL_TRIANGLES);
	for (float ang=2*M_PI; ang>0; ang-=0.1) {
		vector3d v1(size*sin(ang), yPos, size*cos(ang));
		vector3d v2(size*sin(ang+0.1), yPos, size*cos(ang+0.1));
		v1 = v1.Normalized();
		v2 = v2.Normalized();
		if (southPole)
			SphereTriSubdivide(center, v2, v1, 4);
		else
			SphereTriSubdivide(center, v1, v2, 4);
	}
	glEnd();


	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

struct ColRangeObj_t {
	float baseCol[4]; float modCol[4]; float modAll;

	void GenCol(float col[4], MTRand &rng) const {
		float ma = 1 + (rng.Double(modAll*2)-modAll);
		for (int i=0; i<4; i++) col[i] = baseCol[i] + rng.Double(-modCol[i], modCol[i]);
		for (int i=0; i<3; i++) col[i] = CLAMP(ma*col[i], 0, 1);
	}
};

struct GasGiantDef_t {
	int hoopMin, hoopMax; float hoopWobble;
	int blobMin, blobMax;
	float poleMin, poleMax; // size range in radians. zero for no poles.
	float ringProbability;
	ColRangeObj_t ringCol;
	ColRangeObj_t bodyCol;
	ColRangeObj_t hoopCol;
	ColRangeObj_t blobCol;
	ColRangeObj_t poleCol;
};

static GasGiantDef_t ggdefs[] = {
{
	/* jupiter */
	30, 40, 0.05,
	20, 30,
	0, 0,
	0.5,
	{ { .61,.48,.384,.1 }, {0,0,0,.9}, 0.3 },
	{ { .99,.76,.62,1 }, { 0,.1,.1,0 }, 0.3 },
	{ { .99,.76,.62,.5 }, { 0,.1,.1,0 }, 0.3 },
	{ { .99,.76,.62,1 }, { 0,.1,.1,0 }, 0.7 },
}, {
	/* saturnish */
	10, 15, 0.0,
	8, 20, // blob range
	0.2, 0.2, // pole size
	0.5,
	{ { .61,.48,.384,.1 }, {0,0,0,.9}, 0.3 },
	{ { .87, .68, .39, 1 }, { 0,0,0,0 }, 0.1 },
	{ { .87, .68, .39, 1 }, { 0,0,0,0 }, 0.1 },
	{ { .87, .68, .39, 1 }, { 0,0,0,0 }, 0.1 },
	{ { .77, .58, .29, 1 }, { 0,0,0,0 }, 0.1 },
}, {
	/* neptunish */
	3, 6, 0.0,
	2, 6,
	0, 0,
	0.5,
	{ { .61,.48,.384,.1 }, {0,0,0,.9}, 0.3 },
	{ { .31,.44,.73,1 }, {0,0,0,0}, .05}, // body col
	{ { .31,.44,.73,0.5 }, {0,0,0,0}, .1},// hoop col
	{ { .21,.34,.54,1 }, {0,0,0,0}, .05},// blob col
}, {
	/* uranus-like *wink* */
	0, 0, 0.0,
	0, 0,
	0, 0,
	0.5,
	{ { .61,.48,.384,.1 }, {0,0,0,.9}, 0.3 },
	{ { .70,.85,.86,1 }, {.1,.1,.1,0}, 0 },
	{ { .70,.85,.86,1 }, {.1,.1,.1,0}, 0 },
	{ { .70,.85,.86,1 }, {.1,.1,.1,0}, 0 },
	{ { .70,.85,.86,1 }, {.1,.1,.1,0}, 0 }
}, {
	/* brown dwarf-like */
	0, 0, 0.05,
	10, 20,
	0, 0,
	0.5,
	{ { .81,.48,.384,.1 }, {0,0,0,.9}, 0.3 },
	{ { .4,.1,0,1 }, {0,0,0,0}, 0.1 },
	{ { .4,.1,0,1 }, {0,0,0,0}, 0.1 },
	{ { .4,.1,0,1 }, {0,0,0,0}, 0.1 },
},
};

#define PLANET_AMBIENT	0.1

static void SetMaterialColor(const float col[4])
{
	float mambient[4];
	mambient[0] = col[0]*PLANET_AMBIENT;
	mambient[1] = col[1]*PLANET_AMBIENT;
	mambient[2] = col[2]*PLANET_AMBIENT;
	mambient[3] = col[3];
	glMaterialfv (GL_FRONT, GL_AMBIENT, mambient);
	glMaterialfv (GL_FRONT, GL_DIFFUSE, col);
}

/*
 * 1980s graphics
 */
#define GEOSPLIT	4
#define GEOROUGHNESS	0.7
static const float _yes[] = { 1.0, 0.5, 0.25, 0.126, 0.0625, 0.03125 };
static void SubdivideTriangularContinent2(std::vector<vector3d> &verts, int sidx, int eidx, int depth, MTRand &rng)
{
	vector3d &v1 = verts[sidx];
	vector3d &v2 = verts[eidx];
	if (depth > 0) {
		int midx = (sidx+eidx)>>1;
		vector3d c = vector3d::Cross(v2-v1,v1).Normalized();
		c *= rng.Double(1.0);
		verts[midx] = (v1+v2+0.7*_yes[GEOSPLIT-depth]*c).Normalized();
		SubdivideTriangularContinent2(verts, sidx, midx, depth-1, rng);
		SubdivideTriangularContinent2(verts, midx, eidx, depth-1, rng);
	}
}

static void SubdivideVeryLongTri(vector3d &tip, vector3d &v1, vector3d &v2, int bits)
{
	vector3d v;
	vector3d tip2v1 = v1-tip;
	vector3d tip2v2 = v2-tip;

	tip2v1 *= 1.0/bits;
	tip2v2 *= 1.0/bits;

	// tip triangle
	glBegin(GL_TRIANGLES);
	glNormal3dv(&tip.x);
	glVertex3dv(&tip.x);
	v = (tip+tip2v1).Normalized();
	glNormal3dv(&v.x);
	glVertex3dv(&v.x);
	v = (tip+tip2v2).Normalized();
	glNormal3dv(&v.x);
	glVertex3dv(&v.x);
	glEnd();

	glBegin(GL_QUADS);
	for (int i=1; i<bits; i++) {
		v = (tip+(tip2v1*i)).Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
		v = (tip+(tip2v1*(i+1))).Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
		v = (tip+(tip2v2*(i+1))).Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
		v = (tip+(tip2v2*i)).Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
	}
	glEnd();
}

static void SphereBlobTess(vector3d &centre, std::vector<vector3d> &edgeVerts)
{
	const int s = edgeVerts.size();
	std::vector<char> vDead(s);
	int iters =0;
	int v1 = 0;
	int v2 = 1;
	int v3 = 2;
	do {
		vector3d v2dir = edgeVerts[v3] - edgeVerts[v2];
		vector3d v1norm = vector3d::Cross(edgeVerts[v1], edgeVerts[v2] - edgeVerts[v1]);

		const float dot = vector3d::Dot(v1norm, v2dir);

		if (dot >= 0.0) {
			glBegin(GL_TRIANGLES);
			// makes like a billion tris...
			SphereTriSubdivide(edgeVerts[v1], edgeVerts[v2], edgeVerts[v3], 3);
			glEnd();
			vDead[v2] = 1;

			v2 = v3;
			do { v3 = (v3+1)%s; } while (vDead[v3]);
		} else {
			v1 = v2;
			v2 = v3;
			do { v3 = (v3+1)%s; } while (vDead[v3]);
		}
		if (++iters > 1000) break;
	} while ((v1!=v2)&&(v2!=v3)&&(v3!=v1));
	int notDead = 0;
	for (unsigned int i=0; i<vDead.size(); i++) if (!vDead[i]) notDead++;
	//if (notDead > 2) printf("Strange sphere tesselator: %d not dead (%d iters)\n", notDead, iters);
}

static int exp2i(int poo) { int n=2; while (--poo) n*=2; return n; }
static void MakeContinent(matrix4x4d &rot, float scale, MTRand &rng)
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
		
	const int nvps = exp2i(GEOSPLIT);
	const int numVertices = nvps*3 + 1;
	// this is a continent centred on the north pole, of size roughly 45
	// degrees in each direction (although it is based on a triangle, so
	// the actual shape will be a load of crap)
	vector3d v1(0,1,scale*1);
	vector3d v2(scale*sin(2*M_PI/3.0),1,scale*cos(2*M_PI/3.0));
	vector3d v3(-scale*sin(2*M_PI/3.0),1,scale*cos(2*M_PI/3.0));
	v1 = (rot*v1).Normalized();
	v2 = (rot*v2).Normalized();
	v3 = (rot*v3).Normalized();
	std::vector<vector3d> edgeVerts(numVertices);
	edgeVerts[0] = v1;
	edgeVerts[nvps] = v2;
	edgeVerts[2*nvps] = v3;
	edgeVerts[3*nvps] = v1;
	SubdivideTriangularContinent2(edgeVerts, 0, nvps, GEOSPLIT, rng);
	SubdivideTriangularContinent2(edgeVerts, nvps, 2*nvps, GEOSPLIT, rng);
	SubdivideTriangularContinent2(edgeVerts, 2*nvps, 3*nvps, GEOSPLIT, rng);

	vector3d centre = (v1+v2+v3).Normalized();
	SphereBlobTess(centre, edgeVerts);
	
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_NORMALIZE);
}

/*
 * draws at north pole
 */
void DrawCircle(float rad)
{
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);

	glBegin(GL_TRIANGLE_FAN);
	glNormal3d(0,1,0);
	glVertex3d(0,1,0);
	for (double theta=0; theta<M_PI*2; theta+=0.1) {
		vector3d v(rad*sin(theta), 1, rad*cos(theta));
		v = v.Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
	}
	{
		vector3d v(0,1,rad);
		v = v.Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
	}
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

/*
 * draws at north pole
 */
static void DrawEjecta(float rad1, float rad2, int points) // that's a star shape
{
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);

	double step = 2*M_PI/points;

	for (int p=0; p<points; p++) {
		double ang0 = step*p;
		double ang1 = step*(p+1);
		double ang2 = (ang0+ang1)*.5;
		vector3d v1(rad1*sin(ang0), 1, rad1*cos(ang0));
		vector3d v2(rad2*sin(ang2), 1, rad2*cos(ang2));
		vector3d v3(rad1*sin(ang1), 1, rad1*cos(ang1));
		v1 = v1.Normalized();
		v2 = v2.Normalized();
		v3 = v3.Normalized();
		
		SubdivideVeryLongTri(v2, v3, v1, 6);

		glBegin(GL_TRIANGLES);
		// tri to center
		glNormal3dv(&v1.x);
		glVertex3dv(&v1.x);
		glNormal3dv(&v3.x);
		glVertex3dv(&v3.x);
		glNormal3d(0,1,0);
		glVertex3d(0,1,0);
		glEnd();
	}
	
	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

/*
 * draws at north pole
 */
void DrawHollowCircle(float rad1, float rad2)
{
	glPushAttrib(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_BLEND);

	glBegin(GL_TRIANGLE_STRIP);
	for (double theta=0; theta<2*M_PI; theta+=0.1) {
		vector3d v(rad1*sin(theta), 1, rad1*cos(theta));
		v = v.Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);

		v = vector3d(rad2*sin(theta), 1, rad2*cos(theta));
		v = v.Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
	}
	{
		vector3d v(0,1,rad1);
		v = v.Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);

		v = vector3d(0,1,rad2);
		v = v.Normalized();
		glNormal3dv(&v.x);
		glVertex3dv(&v.x);
	}
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_NORMALIZE);
	glPopAttrib();
}

void Planet::DrawRockyPlanet()
{
	int n;
	float r, tmp;
	matrix4x4d rot;
	float col[4], col2[4];
//	MTRand rng((int)Pi::GetGameTime());
	MTRand rng(sbody->seed);
	float darkblue[4] = { .05, .05, .2, 1 };
	float blue[4] = { .2, .2, 1, 1 };
	float green[4] = { .2, .8, .2, 1 };
	float white[4] = { 1, 1, 1, 1 };
	ColRangeObj_t barrenBodyCol = { { .3,.3,.3,1 },{0,0,0,0},.3 };
	ColRangeObj_t barrenContCol = { { .2,.2,.2,1 },{0,0,0,0},.3 };
	ColRangeObj_t barrenEjectaCraterCol = { { .5,.5,.5,1 },{0,0,0,0},.2 };

	switch (sbody->type) {
	case SBody::TYPE_PLANET_DWARF:
	case SBody::TYPE_PLANET_SMALL:
		barrenBodyCol.GenCol(col2, rng);
		SetMaterialColor(col2);
		DrawShittyRoundCube(1.0f);

		n = rng.Int32(3,10);
		barrenContCol.GenCol(col, rng);
		SetMaterialColor(col);
		while (n--) {
			rot = matrix4x4d::RotateXMatrix(rng.Double(M_PI/2));
			rot.RotateZ(rng.Double(M_PI*2));
			MakeContinent(rot, rng.Double(0.05,0.2), rng);
		}

		SetMaterialColor(col);
		n = rng.Int32(50,100);
		while (n--) {
			barrenContCol.GenCol(col, rng);
			r = rng.Double(0.02, 0.1);
			glPushMatrix();
			vector3d rx(rng.Double(1.0)-.5, rng.Double(1.0)-.5, rng.Double(1.0)-.5);
			rx = rx.Normalized();
			glRotatef(rng.Double(0, 360), rx.x, rx.y, rx.z);

			tmp = rng.Double(1.0);
			if (tmp < .46) {
				DrawCircle(r);
			} else if (tmp < .92) {
				//DrawHollowCircle(r, r*1.3);
				DrawCircle(r*1.3);
				// erm yeah
				SetMaterialColor(col2);
				DrawCircle(r);
				SetMaterialColor(col);
			} else {
				barrenEjectaCraterCol.GenCol(col, rng);
				SetMaterialColor(col);
				DrawEjecta(r*0.6, 3*r, 6);
				SetMaterialColor(col2);
				DrawCircle(r*0.4);
			}
			glPopMatrix();
		}
		break;
	
	case SBody::TYPE_PLANET_WATER:
	case SBody::TYPE_PLANET_WATER_THICK_ATMOS:
		SetMaterialColor(darkblue);
		DrawShittyRoundCube(1.0f);
		
		n = rng.Int32(3,10);
		while (n--) {
			barrenBodyCol.GenCol(col2, rng);
			SetMaterialColor(col2);
			rot = matrix4x4d::RotateXMatrix(-M_PI/2+rng.Double(-M_PI/3, M_PI/3));
			rot.RotateZ(rng.Double(M_PI*2));
			MakeContinent(rot, rng.Double(0.1,0.5), rng);
		}
		/* poles */
		SetMaterialColor(white);
		rot = matrix4x4d::Identity();
		MakeContinent(rot, 0.25, rng);
		rot = matrix4x4d::RotateXMatrix(M_PI);
		MakeContinent(rot, 0.25, rng);
		break;
		
	case SBody::TYPE_PLANET_INDIGENOUS_LIFE:
		SetMaterialColor(blue);
		DrawShittyRoundCube(1.0f);
		
		n = rng.Int32(3,10);
		while (n--) {
			SetMaterialColor(green);
			rot = matrix4x4d::RotateXMatrix(-M_PI/2+rng.Double(-M_PI/3, M_PI/3));
			rot.RotateZ(rng.Double(M_PI*2));
			MakeContinent(rot, rng.Double(0.1,0.5), rng);
		}
		/* poles */
		SetMaterialColor(white);
		rot = matrix4x4d::Identity();
		MakeContinent(rot, 0.25, rng);
		rot = matrix4x4d::RotateXMatrix(M_PI);
		MakeContinent(rot, 0.25, rng);
		break;
	default:
		barrenBodyCol.GenCol(col, rng);
		SetMaterialColor(col);
		DrawShittyRoundCube(1.0f);
		break;
	}
}

void Planet::DrawGasGiant()
{
//	MTRand rng((int)Pi::GetGameTime());
	MTRand rng(sbody->seed+9);
	float col[4];
	
	// just use a random gas giant flavour for the moment
	GasGiantDef_t &ggdef = ggdefs[rng.Int32(0,3)];

	ggdef.bodyCol.GenCol(col, rng);
	SetMaterialColor(col);
	DrawShittyRoundCube(1.0f);
	
	int n = rng.Int32(ggdef.hoopMin, ggdef.hoopMax);

	while (n-- > 0) {
		ggdef.hoopCol.GenCol(col, rng);
		SetMaterialColor(col);
		DrawHoop(rng.Double(0.9*M_PI)-0.45*M_PI, rng.Double(0.25), ggdef.hoopWobble, rng);
	}

	n = rng.Int32(ggdef.blobMin, ggdef.blobMax);
	while (n-- > 0) {
		float a = rng.Double(0.01, 0.03);
		float b = a+rng.Double(0.2)+0.1;
		ggdef.blobCol.GenCol(col, rng);
		SetMaterialColor(col);
		DrawBlob(rng.Double(-0.3*M_PI, 0.3*M_PI), rng.Double(2*M_PI), a, b);
	}

	if (ggdef.poleMin != 0) {
		float size = rng.Double(ggdef.poleMin, ggdef.poleMax);
		ggdef.poleCol.GenCol(col, rng);
		SetMaterialColor(col);
		DrawPole(1.0, size);
		DrawPole(-1.0, size);
	}
	
	if (rng.Double(1.0) < ggdef.ringProbability) {
		float pos = rng.Double(1.2,1.7);
		float end = pos + rng.Double(0.1, 1.0);
		end = MIN(end, 2.5);
		while (pos < end) {
			float size = rng.Double(0.1);
			ggdef.ringCol.GenCol(col, rng);
			DrawRing(pos, pos+size, col);
			pos += size;
		}
	}
}

static void _DrawAtmosphere(double rad1, double rad2, vector3d &pos, const float col[4])
{
	glPushMatrix();
	// face the camera dammit
	vector3d zaxis = (-pos).Normalized();
	vector3d xaxis = (vector3d::Cross(zaxis, vector3d(0,1,0))).Normalized();
	vector3d yaxis = vector3d::Cross(zaxis,xaxis);
	matrix4x4d rot = matrix4x4d::MakeRotMatrix(xaxis, yaxis, zaxis).InverseOf();
	glMultMatrixd(&rot[0]);

	const double angStep = M_PI/32;
	// find angle player -> centre -> tangent point
	// tangent is from player to surface of sphere
	float tanAng = acos(rad1 / pos.Length());

	// then we can put the fucking atmosphere on the horizon
	vector3d r1(0.0, 0.0, rad1);
	vector3d r2(0.0, 0.0, rad2);
	rot = matrix4x4d::RotateYMatrix(tanAng);
	r1 = rot * r1;
	r2 = rot * r2;

	rot = matrix4x4d::RotateZMatrix(angStep);

	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBegin(GL_TRIANGLE_STRIP);
	for (float ang=0; ang<2*M_PI; ang+=angStep) {
		glColor4fv(col);
		glVertex3dv(&r1.x);
		glColor4f(0,0,0,0);
		glVertex3dv(&r2.x);
		r1 = rot * r1;
		r2 = rot * r2;
	}
	
	glEnd();
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void Planet::DrawAtmosphere(double rad, vector3d &pos)
{
	if (sbody->type == SBody::TYPE_PLANET_SMALL) {
		const float c[4] = { .2, .2, .3, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_CO2_THICK_ATMOS) {
		const float c[4] = { .8, .8, .8, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.1, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_CO2) {
		const float c[4] = { .5, .5, .5, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_METHANE_THICK_ATMOS) {
		const float c[4] = { .2, .6, .3, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.1, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_METHANE) {
		const float c[4] = { .2, .6, .3, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_HIGHLY_VOLCANIC) {
		const float c[4] = { .5, .2, .2, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_WATER_THICK_ATMOS) {
		const float c[4] = { .8, .8, .8, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.1, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_WATER) {
		const float c[4] = { .2, .2, .4, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
	else if (sbody->type == SBody::TYPE_PLANET_INDIGENOUS_LIFE) {
		const float c[4] = { .2, .2, .5, .8 };
		_DrawAtmosphere(rad*0.99, rad*1.05, pos, c);
	}
}


// tri edge lengths
#define GEOPATCH_SUBDIVIDE_AT_CAMDIST	1.0
#define GEOPATCH_MAX_DEPTH	12
// should be power of two + 1
#define GEOPATCH_EDGELEN	17

#define PRINT_VECTOR(_v) printf("%.2f,%.2f,%.2f\n", (_v).x, (_v).y, (_v).z);

class GeoPatch {
public:
	vector3d v[4];
	GLdouble *vertices;
	GLdouble *normals;
	GLuint *indices;
	GeoPatch *kids[4];
	GeoPatch *parent;
	GeoPatch *edgeFriend[4]; // [0]=v01, [1]=v12, [2]=v20
	int m_numVertices;
	int m_depth;
	GeoPatch() {
		memset(this, 0, sizeof(GeoPatch));
	}
	GeoPatch(vector3d v0, vector3d v1, vector3d v2, vector3d v3) {
		memset(this, 0, sizeof(GeoPatch));
		v[0] = v0; v[1] = v1; v[2] = v2; v[3] = v3;
	}
	~GeoPatch() {
		for (int i=0; i<4; i++) if (kids[i]) delete kids[i];
		if (vertices) delete vertices;
		if (indices) delete indices;
	}
	void GetEdgeVertices(int edge, GLdouble ev[3*GEOPATCH_EDGELEN]) {
	}
	void SetEdgeVertices(int edge, GLdouble ev[3*GEOPATCH_EDGELEN]) {
	}
	void SetEdgeVerticesFlipped(int edge, GLdouble ev[3*GEOPATCH_EDGELEN]) {
	}
	int GetEdgeIdxOf(GeoPatch *e) {
		for (int i=0; i<4; i++) {
			if (edgeFriend[i] == e) return i;
		}
		return -1;
	}

	void Generate() {
		if (!vertices) {
			m_numVertices = GEOPATCH_EDGELEN*GEOPATCH_EDGELEN;
			vertices = new GLdouble[3*m_numVertices];
			normals = new GLdouble[3*m_numVertices];
			GLdouble *vts = vertices;
			for (int x=0; x<GEOPATCH_EDGELEN; x++) {
				for (int y=0; y<GEOPATCH_EDGELEN; y++) {
					double xpos = x/(double)(GEOPATCH_EDGELEN-1);
					double ypos = y/(double)(GEOPATCH_EDGELEN-1);
					vector3d p = v[0] + xpos*(1.0-ypos)*(v[1]-v[0]) +
						            xpos*ypos*(v[2]-v[0]) +
							    (1.0-xpos)*ypos*(v[3]-v[0]);
					p = p.Normalized();

					double n = noise(64*p.x, 64*p.y, 64*p.z);
					n *= noise(128*p.x, 128*p.y, 128*p.z);
					n *= noise(256*p.x, 256*p.y, 256*p.z);
					n *= noise(512*p.x, 512*p.y, 512*p.z);
					n *= noise(1024*p.x, 1024*p.y, 1024*p.z);
					//n = n*n*noise(10*p.x, 10*p.y, 10*p.z);
					p = p + p*0.1*n;
					vts[0] = p.x; vts[1] = p.y; vts[2] = p.z;
					vts+=3;
				}
			}

			for (int x=0; x<GEOPATCH_EDGELEN-1; x++) {
				for (int y=0; y<GEOPATCH_EDGELEN-1; y++) {
					vector3d a = vector3d(&vertices[3*(x + y*GEOPATCH_EDGELEN)]);
					vector3d b = vector3d(&vertices[3*(x+1 + y*GEOPATCH_EDGELEN)]);
					vector3d c = vector3d(&vertices[3*(x + (y+1)*GEOPATCH_EDGELEN)]);

					vector3d n = vector3d::Cross(c-a, b-a);
					n = n.Normalized();
					GLdouble *vts = &normals[3*(x + y*GEOPATCH_EDGELEN)];
					vts[0] = n.x; vts[1] = n.y; vts[2] = n.z;
				}
			}
			// XXX crap
			for (int y=0; y<GEOPATCH_EDGELEN; y++) {
				GLdouble *poo = &normals[3*(GEOPATCH_EDGELEN-1 + y*GEOPATCH_EDGELEN)];
				*poo = *(poo-3);
				*(poo+1) = *(poo-2);
				*(poo+2) = *(poo-1);
			}
			for (int x=0; x<GEOPATCH_EDGELEN; x++) {
				GLdouble *poo = &normals[3*(x + (GEOPATCH_EDGELEN-1)*GEOPATCH_EDGELEN)];
				*poo = *(poo-3*GEOPATCH_EDGELEN);
				*(poo+1) = *(poo+1-3*GEOPATCH_EDGELEN);
				*(poo+2) = *(poo+2-3*GEOPATCH_EDGELEN);
			}

			/*for (int elen = 1; elen<=GEOPATCH_EDGELEN; elen++) {
				for (int pos=0; pos<elen; pos++) {
					double ucoord, vcoord;
					if (elen == 1) {
						ucoord = 0;
						vcoord = 0;
					} else {
						double jizz = (elen-1)/(double)(GEOPATCH_EDGELEN-1);
						ucoord = jizz*pos/(double)(elen-1);
						vcoord = jizz*((elen-1)-pos)/(double)(elen-1);
					}
					vector3d p = (v[0] + vcoord*(v[1]-v[0]) + ucoord*(v[2]-v[0])).Normalized();
					double n = noise(100*p.x, 100*p.y, 100*p.z);
					n = n*n*noise(10*p.x, 10*p.y, 10*p.z);
					p = p + p*0.01*n;
					vts[0] = p.x; vts[1] = p.y; vts[2] = p.z;
					vts+=3;
				}
			}*/

			assert(vts == &vertices[3*m_numVertices]);
			indices = new GLuint[2*(GEOPATCH_EDGELEN-1)*(GEOPATCH_EDGELEN-1)*3];
			GLuint *idx = indices;
			int wank=0;
			for (int x=0; x<GEOPATCH_EDGELEN-1; x++) {
				for (int y=0; y<GEOPATCH_EDGELEN-1; y++) {
					idx[0] = x + GEOPATCH_EDGELEN*y;
					idx[2] = x+1 + GEOPATCH_EDGELEN*y;
					idx[1] = x + GEOPATCH_EDGELEN*(y+1);
					idx+=3;
					wank++;

					idx[0] = x+1 + GEOPATCH_EDGELEN*y;
					idx[2] = x+1 + GEOPATCH_EDGELEN*(y+1);
					idx[1] = x + GEOPATCH_EDGELEN*(y+1);
					idx+=3;
					wank++;
				}
			}/*
			int abspos = 0;
			int wank  = 0;
			for (int elen = 1; elen<GEOPATCH_EDGELEN; elen++) {
				for (int pos=0; pos<elen; pos++) {
					idx[0] = abspos;
					idx[1] = abspos+elen+1;
					idx[2] = abspos+elen;
					idx+=3;
					wank++;
					if (pos < elen-1) {
						idx[0] = abspos;
						idx[1] = abspos+1;
						idx[2] = abspos+elen+1;
						idx+=3;
						wank++;
					}
					abspos++;
				}
			}*/
			assert(wank == 2*(GEOPATCH_EDGELEN-1)*(GEOPATCH_EDGELEN-1));
#if 0
			/* take edge vertices from adjacent tiles if they
			 * exist, so there are no breaks in the terrain */
			GLdouble ev[3*GEOPATCH_EDGELEN];
			for (int j=0; j<3; j++) {
				GeoPatch *edge = edgeFriend[j];
				if (!edge->vertices) continue;
				int we_are = edge->GetEdgeIdxOf(this);
				if (we_are == -1) {
					continue;
					/// not joined to edge properly. TODO
					//TODO TODO for kids on edge!!
				}
				edge->GetEdgeVertices(we_are, ev);
				if (we_are == 2) {
					assert(j==0);
					// dunno what to do when j=1,2.
					// should it be flipped or what??
				}
				if (we_are == 2) {
					SetEdgeVertices(j, ev);
				} else if (j==2) {
					SetEdgeVertices(j, ev);
				} else {
					SetEdgeVerticesFlipped(j, ev);
				}
			}
#endif
#if 0
			/* XXX some tests to ensure vertices match */
			for (int i=0; i<3; i++) {
				GeoPatch *edge = edgeFriend[i];
				if (edge) {
					int we_are = edge->GetEdgeIdxOf(this);
					assert(v[i] == edge->v[(1+we_are)%3]);
					assert(v[(i+1)%3] == edge->v[(we_are)%3]);
				}
			}
#endif
		}

	}
	void Render(vector3d &campos) {
		Generate();
				
		vector3d centroid = (v[0]+v[1]+v[2]+v[3])*0.25;
		if ((m_depth < GEOPATCH_MAX_DEPTH) &&
		    ((campos - centroid).Length() < (v[0]-v[2]).Length()*GEOPATCH_SUBDIVIDE_AT_CAMDIST)) {

			if (!kids[0]) {
				vector3d v01, v12, v23, v30;
				v01 = (v[0]+v[1]).Normalized();
				v12 = (v[1]+v[2]).Normalized();
				v23 = (v[2]+v[3]).Normalized();
				v30 = (v[3]+v[0]).Normalized();
				kids[0] = new GeoPatch(v[0], v01, centroid, v30);
				kids[1] = new GeoPatch(v01, v[1], v12, centroid);
				kids[2] = new GeoPatch(centroid, v12, v[2], v23);
				kids[3] = new GeoPatch(v30, centroid, v23, v[3]);
				kids[0]->m_depth = m_depth+1;
				kids[1]->m_depth = m_depth+1;
				kids[2]->m_depth = m_depth+1;
				kids[3]->m_depth = m_depth+1;
				// hm.. edges.
			/*	kids[0]->edgeFriend[0] = edgeFriend[0]; // XXX giving parent's neighbour...
				kids[0]->edgeFriend[1] = kids[3];
				kids[0]->edgeFriend[2] = edgeFriend[2]; // XXX
				kids[1]->edgeFriend[0] = edgeFriend[0]; // XXX
				kids[1]->edgeFriend[1] = edgeFriend[1]; // XXX
				kids[1]->edgeFriend[2] = kids[3];
				kids[2]->edgeFriend[0] = kids[3];
				kids[2]->edgeFriend[1] = edgeFriend[1]; // XXX
				kids[2]->edgeFriend[2] = edgeFriend[2]; // XXX
				kids[3]->edgeFriend[0] = kids[1];
				kids[3]->edgeFriend[1] = kids[2];
				kids[3]->edgeFriend[2] = kids[0];*/
				kids[0]->parent = kids[1]->parent = kids[2]->parent = kids[3]->parent = this;
			}
			for (int i=0; i<4; i++) kids[i]->Render(campos);
		} else {
			if (kids[0]) {
				for (int i=0; i<4; i++) { delete kids[i]; kids[i] = 0; }
			}
			glShadeModel(GL_SMOOTH);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);
			glVertexPointer(3, GL_DOUBLE, 0, vertices);
			glNormalPointer(GL_DOUBLE, 0, normals);
			glDrawElements(GL_TRIANGLES, 2*(GEOPATCH_EDGELEN-1)*(GEOPATCH_EDGELEN-1)*3, GL_UNSIGNED_INT, indices);
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
		}
	}
};

class GeoSphere {

public:
	GeoSphere() {
		vector3d p1(1,1,1);
		vector3d p2(-1,1,1);
		vector3d p3(-1,-1,1);
		vector3d p4(1,-1,1);
		vector3d p5(1,1,-1);
		vector3d p6(-1,1,-1);
		vector3d p7(-1,-1,-1);
		vector3d p8(1,-1,-1);
		p1 = p1.Normalized();
		p2 = p2.Normalized();
		p3 = p3.Normalized();
		p4 = p4.Normalized();
		p5 = p5.Normalized();
		p6 = p6.Normalized();
		p7 = p7.Normalized();
		p8 = p8.Normalized();

		m_patches[0] = GeoPatch(p1, p2, p3, p4);
		m_patches[1] = GeoPatch(p4, p3, p7, p8);
		m_patches[2] = GeoPatch(p1, p4, p8, p5);
		m_patches[3] = GeoPatch(p2, p1, p5, p6);
		m_patches[4] = GeoPatch(p3, p2, p6, p7);
		m_patches[5] = GeoPatch(p8, p7, p6, p5);
		//	for (int j=0; j<3; j++) {
		//		p->edgeFriend[j] = &m_patches[nus_edgecomrade[i][j]];
		//	}
		for (int i=0; i<6; i++) {
			m_patches[i].Generate();
		}
	}
	void Render(vector3d campos) {
		for (int i=0; i<6; i++) {
			m_patches[i].Render(campos);
		}
	}
	GeoPatch m_patches[6];
};

void Planet::Render(const Frame *a_camFrame)
{
	glPushMatrix();

	if (!m_geosphere) m_geosphere = new GeoSphere();
	
	double rad = sbody->GetRadius();
	matrix4x4d ftran;
	Frame::GetFrameTransform(GetFrame(), a_camFrame, ftran);
	vector3d fpos = ftran * GetPosition();

	double apparent_size = rad / fpos.Length();
	double len = fpos.Length();
	double origLen = len;

	do {
		rad *= 0.25;
		fpos = 0.25*fpos;
		len *= 0.25;
	} while ((len-rad)*0.25 > 4*WORLDVIEW_ZNEAR);
		
	glTranslatef(fpos.x, fpos.y, fpos.z);
	glColor3f(1,1,1);

	if (apparent_size < 0.001) {
		if (crudDList) {
			glDeleteLists(crudDList, 1);
			crudDList = 0;
		}
		/* XXX WRONG. need to pick light from appropriate turd. */
		GLfloat col[4];
		glGetLightfv(GL_LIGHT0, GL_DIFFUSE, col);
		// face the camera dammit
		vector3d zaxis = fpos.Normalized();
		vector3d xaxis = vector3d::Cross(vector3d(0,1,0), zaxis).Normalized();
		vector3d yaxis = vector3d::Cross(zaxis,xaxis);
		matrix4x4d rot = matrix4x4d::MakeRotMatrix(xaxis, yaxis, zaxis).InverseOf();
		glMultMatrixd(&rot[0]);

		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		
		glEnable(GL_BLEND);
		glColor4f(col[0], col[1], col[2], 1);
		glBegin(GL_TRIANGLE_FAN);
		glVertex3f(0,0,0);
		glColor4f(col[0],col[1],col[2],0);
		
		const float spikerad = 0.005*len +  1e1*(1.0*sbody->GetRadius()*len)/origLen;
		{
			/* bezier with (0,0,0) control points */
			vector3f p0(0,spikerad,0), p1(spikerad,0,0);
			float t=0.1; for (int i=1; i<10; i++, t+= 0.1f) {
				vector3f p = (1-t)*(1-t)*p0 + t*t*p1;
				glVertex3fv(&p[0]);
			}
		}
		{
			vector3f p0(spikerad,0,0), p1(0,-spikerad,0);
			float t=0.1; for (int i=1; i<10; i++, t+= 0.1f) {
				vector3f p = (1-t)*(1-t)*p0 + t*t*p1;
				glVertex3fv(&p[0]);
			}
		}
		{
			vector3f p0(0,-spikerad,0), p1(-spikerad,0,0);
			float t=0.1; for (int i=1; i<10; i++, t+= 0.1f) {
				vector3f p = (1-t)*(1-t)*p0 + t*t*p1;
				glVertex3fv(&p[0]);
			}
		}
		{
			vector3f p0(-spikerad,0,0), p1(0,spikerad,0);
			float t=0.1; for (int i=1; i<10; i++, t+= 0.1f) {
				vector3f p = (1-t)*(1-t)*p0 + t*t*p1;
				glVertex3fv(&p[0]);
			}
		}
		glEnd();
		glDisable(GL_BLEND);

		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
	} else {
		vector3d campos = -fpos;
		ftran.ClearToRotOnly();
		campos = ftran.InverseOf() * campos;
		glMultMatrixd(&ftran[0]);
		const float poo[4] = { 1,1,1,1};
		SetMaterialColor(poo);
		glEnable(GL_NORMALIZE);
		glShadeModel(GL_FLAT);
		glPushMatrix();
		glScalef(rad,rad,rad);
		campos = campos * (1.0/rad);
		m_geosphere->Render(campos);
		glPopMatrix();
		glShadeModel(GL_SMOOTH);
		glDisable(GL_NORMALIZE);

		/*
		ftran.ClearToRotOnly();
		glMultMatrixd(&ftran[0]);

		if (!crudDList) {
			crudDList = glGenLists(1);
			glNewList(crudDList, GL_COMPILE);
			// this is a rather brittle test..........
			if (sbody->type < SBody::TYPE_PLANET_DWARF) {
				DrawGasGiant();
			} else {
				DrawRockyPlanet();
			}
			glEndList();
		}
		glPushMatrix();
		glScalef(rad,rad,rad);
		glCallList(crudDList);
		glPopMatrix();
*/

		fpos = ftran.InverseOf() * fpos;

		DrawAtmosphere(rad, fpos);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	glPopMatrix();
}

void Planet::SetFrame(Frame *f)
{
	if (GetFrame()) {
		GetFrame()->SetPlanetGeom(0, 0);
	}
	Body::SetFrame(f);
	if (f) {
		GetFrame()->SetPlanetGeom(GetRadius(), this);
	}
}


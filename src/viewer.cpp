#include "viewer.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "SOIL/SOIL.h"
//#include <FreeImage.h>

#include "texthandle.h"
#include "pmx.h"
#include "vmd.h"
#include "shader.h"
#include "pmxvLogger.h"

#include "motioncontroller.h"
#include "bulletphysics.h"
#include "mmdphysics.h"

#include "sound.h"

#include "glfw_func_callbacks.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//#define MODELDUMP true

using namespace std;


Viewer::Viewer(string modelPath, string motionPath,string musicPath)
{
	int index=modelPath.rfind("/");
	string modelFilePath,modelFolderPath;
	
	if(index==-1)
	{
		modelFolderPath="";
		modelFilePath=modelPath;
	}
	else
	{
		modelFilePath=modelPath.substr(index);
		modelFolderPath=modelPath.substr(0,index);
	}
	
	pmxInfo=&readPMX(modelFolderPath,modelFilePath);
	vmdInfo=&readVMD(motionPath);
	//pmxInfo=&readPMX("data/model/gumiv3/","GUMI_V3.pmx");
	//vmdInfo=&readVMD("data/motion/Watashi no Jikan/私の時間_short_Lat式ミク.vmd");
	
	initGLFW();
	
	ifstream test("shaders/model.vert");
	if(!test.is_open())
	{
		shaderProgram=loadShaders(DATA_PATH"/shaders/model.vert",DATA_PATH"/shaders/model.frag");
	}
	else
	{
		shaderProgram=loadShaders("shaders/model.vert","shaders/model.frag");
	}
	test.close();
	
	
	
	glUseProgram(shaderProgram);
	loadTextures();
	initBuffers();
	initUniformVarLocations();
	
	MVP_loc = glGetUniformLocation(shaderProgram, "MVP");
    
	//Set OpenGL render settings
    glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPointSize(5.0);
	//glClearDepth(1.0f);
	
	//glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
	glClearColor(1,1,1,1);
	//glDisable(GL_MULTISAMPLE);
	
	//Setup MotionController, Physics
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[VertexArrayBuffer]);
	motionController=new VMDMotionController(*pmxInfo,*vmdInfo,shaderProgram);
	
	
	ifstream test2("shaders/bulletDebug.vert");
	GLuint btDebugShaderProgram;
	if(!test2.is_open())
	{
		btDebugShaderProgram=loadShaders(DATA_PATH"/shaders/bulletDebug.vert",DATA_PATH"/shaders/bulletDebug.frag");
	}
	else
	{
		btDebugShaderProgram=loadShaders("shaders/bulletDebug.vert","shaders/bulletDebug.frag");
	}
	//GLuint btDebugShaderProgram=loadShaders(DATA_PATH"/shaders/bulletDebug.vert",DATA_PATH"/shaders/bulletDebug.frag");
	
	initSound(musicPath);
	
	bulletPhysics = new BulletPhysics(btDebugShaderProgram);
	glUseProgram(shaderProgram); //restore GL shader program to Viewer's shader program after initializing BulletPhysic's debugDrawer
	mmdPhysics = new MMDPhysics(*pmxInfo,motionController,bulletPhysics);
	
	motionController->updateVertexMorphs();
	motionController->updateBoneAnimation();
	
	//Initialize timer variables
	startTime = glfwGetTime();
	ticks=0;
	
	modelTranslate=glm::vec3(0.0f,-10.0f,0.0f);
}

void Viewer::handleLogic()
{
	bool doPhysics=true;
	if(glfwGetKey('Q')==GLFW_PRESS)
	{
		doPhysics=false;
	}
	
	//if(glfwGetKey('A')==GLFW_RELEASE)
	{
		motionController->advanceTime();
		
		//Debug- hold model in bind pose
		//holdModelInBindPose();
		
		motionController->updateVertexMorphs();
		motionController->updateBoneAnimation();
		mmdPhysics->updateBones(doPhysics);
		
		glUseProgram(bulletPhysics->debugDrawer->shaderProgram);
		setCamera(bulletPhysics->debugDrawer->MVPLoc);
		glUseProgram(shaderProgram);
		setCamera(MVP_loc);
	}
}

void Viewer::render()
{
	if(glfwGetKey('A')==GLFW_RELEASE)
	{
		drawModel(true); //draw model edges
		drawModel(false); //draw model
	}
	
	if(glfwGetKey('S')==GLFW_PRESS)
	{
		bulletPhysics->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
		bulletPhysics->DebugDrawWorld();
	}
	else if(glfwGetKey('D')==GLFW_PRESS)
	{
		bulletPhysics->setDebugMode(btIDebugDraw::DBG_DrawAabb);
		bulletPhysics->DebugDrawWorld();
	}
	else if(glfwGetKey('F')==GLFW_PRESS)
	{
		bulletPhysics->setDebugMode(btIDebugDraw::DBG_DrawConstraints);
		bulletPhysics->DebugDrawWorld();
	}
	else if(glfwGetKey('G')==GLFW_PRESS)
	{
		bulletPhysics->setDebugMode(btIDebugDraw::DBG_DrawConstraintLimits);
		bulletPhysics->DebugDrawWorld();
	}
	glUseProgram(shaderProgram); //Restore shader program and buffer's to Viewer's after drawing Bullet debug
	glBindVertexArray(VAOs[Vertices]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[VertexArrayBuffer]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[VertexIndexBuffer]);
	
	//drawIKMarkers();

	//glFinish();
	//glDrawBuffer(RecordBuffer,Buffers);
	//glReadPixels(0,0,1920,1080,GL_RGB,
}

void Viewer::run()
{
	while(glfwGetKey( GLFW_KEY_ESC ) != GLFW_PRESS && glfwGetWindowParam( GLFW_OPENED ))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		startTime = glfwGetTime();
		
		handleEvents();
		handleLogic();
		render();
		
		glfwSwapBuffers();
	}
}

void Viewer::setCamera(GLuint MVPLoc)
{
	glm::mat4 Projection = glm::perspective(45.0f, 16.0f/9.0f, 0.1f, 100.0f);
	// Camera matrix
	glm::mat4 View       = glm::lookAt(
		glm::vec3(cameraPosition.x,cameraPosition.y,-cameraPosition.z), // Camera is at (4,3,3), in World Space
		cameraTarget, // and looks at the origin
		glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
	);
	//View= glm::rotate(0.0f,0.0f,0.0f,1.0f)* View;
	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model = glm::translate(modelTranslate.x, modelTranslate.y, modelTranslate.z);
	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 MVP = Projection * View * Model;
	
	glUniformMatrix4fv(MVPLoc, 1, GL_FALSE, &MVP[0][0]);
}

void Viewer::drawModel(bool drawEdges)
{
	//Bind VAO and related Buffers
	glBindVertexArray(VAOs[Vertices]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[VertexArrayBuffer]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[VertexIndexBuffer]);
	
	if(drawEdges)
	{
		glDisable(GL_BLEND);
		glCullFace(GL_FRONT);
        glUniform1i(uniformVars[uIsEdge], 1);
        
        glDisable(GL_DEPTH_TEST);
	}
	else
	{        
        glEnable(GL_BLEND);
		glCullFace(GL_BACK);
		glUniform1i(uniformVars[uIsEdge], 0);
		
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_DST_ALPHA);
	}
	
	
	glm::vec3 halfVector=glm::normalize(cameraPosition-cameraTarget);
	halfVector.z=-halfVector.z;
	GLuint halfVectorLoc=glGetUniformLocation(shaderProgram, "halfVector");
	
	glm::vec3 lightDirection=glm::normalize(glm::vec3(0.3,1.0,2.0));
	GLuint lightDirectionLoc=glGetUniformLocation(shaderProgram, "lightDirection");
	
	int faceCount=0;
	for(int m=0; m<pmxInfo->material_continuing_datasets; ++m) //pmxInfo->material_continuing_datasets
	{
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,textures[pmxInfo->materials[m]->textureIndex]);
		glUniform1iARB(uniformVars[uTextureSampler], 0);
		
		if((int)pmxInfo->materials[m]->sphereMode>0)
		{
			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,textures[pmxInfo->materials[m]->sphereIndex]);
			glUniform1iARB(uniformVars[uSphereSampler], 1);
		}
		
		if((int)pmxInfo->materials[m]->shareToon==0)
		{
			glActiveTexture(GL_TEXTURE2);
			glEnable(GL_TEXTURE_2D);
			
			glBindTexture(GL_TEXTURE_2D,textures[pmxInfo->materials[m]->toonTextureIndex]);
			glUniform1iARB(uniformVars[uToonSampler], 2);
		}
		else if((int)pmxInfo->materials[m]->shareToon==1)
		{
			glActiveTexture(GL_TEXTURE2);
			glEnable(GL_TEXTURE_2D);
			
			glBindTexture(GL_TEXTURE_2D,textures[textures.size()-11+pmxInfo->materials[m]->shareToonTexture]);
			glUniform1iARB(uniformVars[uToonSampler], 2);
		}
		
		
		glUniform3fv(uniformVars[uAmbient],1,(GLfloat*)&pmxInfo->materials[m]->ambient);
		glUniform4fv(uniformVars[uDiffuse],1,(GLfloat*)&pmxInfo->materials[m]->diffuse);
		glUniform3fv(uniformVars[uSpecular],1,(GLfloat*)&pmxInfo->materials[m]->specular);
		
		glUniform1f(uniformVars[uShininess],glm::normalize(pmxInfo->materials[m]->shininess));
		glUniform3f(uniformVars[uHalfVector],halfVector.x,halfVector.y,halfVector.z);
		glUniform3f(uniformVars[uLightDirection],lightDirection.x,lightDirection.y,lightDirection.z);
		
		glUniform4fv(uniformVars[uEdgeColor],1,(GLfloat*)&pmxInfo->materials[m]->edgeColor);
		glUniform1f(uniformVars[uEdgeSize],glm::normalize(pmxInfo->materials[m]->edgeSize));
		
		glUniform1i(uniformVars[uSphereMode],pmxInfo->materials[m]->sphereMode);
        
		glDrawElements(GL_TRIANGLES, (pmxInfo->materials[m]->hasFaceNum), GL_UNSIGNED_INT, BUFFER_OFFSET(sizeof(GLuint)*faceCount));
		faceCount+=pmxInfo->materials[m]->hasFaceNum;
	}
}

void Viewer::drawIKMarkers()
{
	glBindVertexArray(VAOs[IKDebugVertices]);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[IKVertexArrayBuffer]);
	
	
	vector<int> targetBoneIndices;
	
	for(int i=0; i<pmxInfo->bone_continuing_datasets; ++i)
	{
		PMXBone *b=pmxInfo->bones[i];
	
		glm::mat4 tmpMatrix=b->calculateGlobalMatrix();
		IKVertexData[i].position.x=tmpMatrix[3][0];
		IKVertexData[i].position.y=tmpMatrix[3][1];
		IKVertexData[i].position.z=tmpMatrix[3][2];
		IKVertexData[i].position.w=tmpMatrix[3][3];
		
		//IKVertexData[i].position.x=b->Global[3][0];
		//IKVertexData[i].position.y=b->Global[3][1];
		//IKVertexData[i].position.z=b->Global[3][2];
		//IKVertexData[i].position.w=b->Global[3][3];

		IKVertexData[i].UV.x=0;
		IKVertexData[i].UV.y=0;
		
		if(b->IK)
		{
			targetBoneIndices.push_back(b->IKTargetBoneIndex);
			
			IKVertexData[i].normal.x=1;
			IKVertexData[i].normal.y=0;
			IKVertexData[i].normal.z=0;
		}
		else
		{
			IKVertexData[i].normal.x=0;
			IKVertexData[i].normal.y=0;
			IKVertexData[i].normal.z=0;
		}

		IKVertexData[i].weightFormula=4;

		IKVertexData[i].boneIndex1=0;
		IKVertexData[i].boneIndex2=0;
		IKVertexData[i].boneIndex3=0;
		IKVertexData[i].boneIndex4=0;

		IKVertexData[i].weight1=0;
		IKVertexData[i].weight2=0;
		IKVertexData[i].weight3=0;
		IKVertexData[i].weight4=0;
		
	}
	
	for(int i=0; i<targetBoneIndices.size(); ++i)
	{
		IKVertexData[ targetBoneIndices[i] ].normal.x=0;
		IKVertexData[ targetBoneIndices[i] ].normal.y=1;
		IKVertexData[ targetBoneIndices[i] ].normal.z=0;
	}
	
	glBufferData(GL_ARRAY_BUFFER, pmxInfo->bone_continuing_datasets*sizeof(VertexData), IKVertexData, GL_DYNAMIC_DRAW);
	
	GLuint Bones_loc=glGetUniformLocation(shaderProgram,"Bones");
	glm::mat4 tmpSkinMatrix[pmxInfo->bone_continuing_datasets];
	
	glUniformMatrix4fv(Bones_loc, pmxInfo->bone_continuing_datasets, GL_FALSE, (const GLfloat*)&tmpSkinMatrix);
	
	glDrawArrays(GL_POINTS, 0, pmxInfo->bone_continuing_datasets);
}


//#define MODELDUMP
void Viewer::initBuffers()
{
	#ifdef MODELDUMP
	ofstream modeldump("modeldump.txt");
	modeldump << "indices:" << endl;
	#endif
	
	//Note: vertex indices are loaded statically, since they do not change.
	//The actual vertex data is loaded dynamically each frame, so its memory is managed by the MotionController.
	GLuint *vertexIndices= (GLuint*) calloc(pmxInfo->face_continuing_datasets,sizeof(GLuint)*3);
	for(int i=0; i<pmxInfo->faces.size(); ++i) //faces.size()
	{
		int j=i*3;
		vertexIndices[j]=pmxInfo->faces[i]->points[0];
		vertexIndices[j+1]=pmxInfo->faces[i]->points[1];
		vertexIndices[j+2]=pmxInfo->faces[i]->points[2];
		
		#ifdef MODELDUMP
		modeldump << vertexIndices[j] << " " << vertexIndices[j+1] << " " << vertexIndices[j+2] << endl;
		#endif
	}
	
	
	#ifdef MODELDUMP
	modeldump << "vertices:" << endl;
	#endif
	
	#ifdef MODELDUMP
	modeldump.close();
	#endif
	
	//Generate all Viewer Buffers
	glGenBuffers(NumBuffers,Buffers);
	
	//init Element Buffer Object
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[VertexIndexBuffer]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, pmxInfo->face_continuing_datasets*sizeof(GLuint)*3, vertexIndices, GL_STATIC_DRAW);
	
	free(vertexIndices);
	
	
	//Init Vertex Array Buffer Object
	glGenVertexArrays(NumVAOs, VAOs);
	glBindVertexArray(VAOs[Vertices]);
	
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[VertexArrayBuffer]);
	
	//Intialize Vertex Attribute Pointers
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(0)); //4=number of components updated per vertex
	glEnableVertexAttribArray(vPosition);
	
	glVertexAttribPointer(vUV, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(GLfloat)*4));
	glEnableVertexAttribArray(vUV);
	
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(GLfloat)*6));
	glEnableVertexAttribArray(vNormal);
	
	glVertexAttribPointer(vBoneIndices, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(GLfloat)*10));
	glEnableVertexAttribArray(vBoneIndices);
	
	glVertexAttribPointer(vBoneWeights, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(GLfloat)*14));
	glEnableVertexAttribArray(vBoneWeights);
	
	glVertexAttribPointer(vWeightFormula, 1, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(GLfloat)*9));
	glEnableVertexAttribArray(vWeightFormula);	
	
	
	//Initialize IK Debug buffers
	IKVertexData = (VertexData*)malloc(pmxInfo->bone_continuing_datasets*sizeof(VertexData));

	glBindVertexArray(VAOs[IKDebugVertices]);
	
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[IKVertexArrayBuffer]);
	
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(0)); //4=number of components updated per vertex
	glEnableVertexAttribArray(vPosition);
	
	glVertexAttribPointer(vUV, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(GLfloat)*4));
	glEnableVertexAttribArray(vUV);
	
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(GLfloat)*6));
	glEnableVertexAttribArray(vNormal);
	
	glVertexAttribPointer(vBoneIndices, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(GLfloat)*10));
	glEnableVertexAttribArray(vBoneIndices);
	
	glVertexAttribPointer(vBoneWeights, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(GLfloat)*14));
	glEnableVertexAttribArray(vBoneWeights);
	
	glVertexAttribPointer(vWeightFormula, 1, GL_FLOAT, GL_FALSE, sizeof(VertexData), BUFFER_OFFSET(sizeof(GLfloat)*9));
	glEnableVertexAttribArray(vWeightFormula);
}

void Viewer::loadTextures()
{
	//Thought about replacing SOIL with FreeImage, but for now sticking with SOIL. Don't fix what isn't broken.
	//Warning: Commented-out FreeImage code below is buggy (DOES NOT WORK)
	
	//FreeImage_Initialise();	
	
	for(int i=0; i<pmxInfo->texture_continuing_datasets; ++i)
	{
		cout<<"Loading "<<pmxInfo->texturePaths[i]<<"...";
		if(pmxInfo->texturePaths[i].substr(pmxInfo->texturePaths[i].size()-3)=="png" || pmxInfo->texturePaths[i].substr(pmxInfo->texturePaths[i].size()-3)=="spa")
		{
			GLuint texture;
			int width, height;
			unsigned char* image;
			string loc=pmxInfo->texturePaths[i];
			
			ifstream test(loc);
			if(!test.is_open())
			{
				cerr<<"Texture file could not be found: "<<loc<<endl;
				//exit(EXIT_FAILURE);
			}
			test.close();
			
			glActiveTexture( GL_TEXTURE0 );
			glGenTextures( 1, &texture );
			glBindTexture( GL_TEXTURE_2D, texture );
				image = SOIL_load_image( loc.c_str(), &width, &height, 0, SOIL_LOAD_RGBA );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image );
			SOIL_free_image_data( image );

			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			
			if(texture==0)
			{
				cerr<<"Texture failed to load: "<<pmxInfo->texturePaths[i]<<endl;
				printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
				exit(EXIT_FAILURE);
			}
			
			cout<<"done"<<endl;
			
			textures.push_back(texture);
		}
		else if(pmxInfo->texturePaths[i].substr(pmxInfo->texturePaths[i].size()-3)=="tga")
		{
			//cerr<<"WARNING: TGA files only mildly tested"<<endl;
			
			/*static GLuint texture = 0;
	
			FIBITMAP *bitmap = FreeImage_Load( FreeImage_GetFileType(pmxInfo->texturePaths[i].c_str(), 0),pmxInfo->texturePaths[i].c_str() );
			FIBITMAP *pImage = FreeImage_ConvertTo24Bits(bitmap);
			
			int nWidth = FreeImage_GetWidth(pImage);
			int nHeight = FreeImage_GetHeight(pImage);
			
			glActiveTexture( GL_TEXTURE0 );
			glGenTextures(1, &texture);

			glBindTexture(GL_TEXTURE_2D, texture);
			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nWidth, nHeight,
			0, GL_BGR, GL_UNSIGNED_BYTE, (void*)FreeImage_GetBits(pImage));
			
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			
			if(texture==0)
			{
				cerr<<"Texture failed to load: "<<pmxInfo->texturePaths[i]<<endl;
				exit(EXIT_FAILURE);
			}

			FreeImage_Unload(bitmap);
			
			textures.push_back(texture);
			
			cout<<"done"<<endl;*/
			
			GLuint texture;
			int width, height, channels;
			unsigned char* image;
			string loc=pmxInfo->texturePaths[i];
			
			ifstream test(loc);
			if(!test.is_open())
			{
				cerr<<"Texture file could not be found: "<<loc<<endl;
				//exit(EXIT_FAILURE);
			}
			test.close();
			
			glActiveTexture( GL_TEXTURE0 );
			glGenTextures( 1, &texture );
			glBindTexture( GL_TEXTURE_2D, texture );
				image = SOIL_load_image( loc.c_str(), &width, &height, &channels, SOIL_LOAD_RGBA );
			
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image );
			SOIL_free_image_data( image );

			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			
			if(texture==0)
			{
				cerr<<"Texture failed to load: "<<pmxInfo->texturePaths[i]<<endl;
				printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
				exit(EXIT_FAILURE);
			}
			
			cout<<"done"<<endl;
			
			textures.push_back(texture);
		}
		else
		{
			GLuint texture;
			int width, height;
			unsigned char* image;
			string loc=pmxInfo->texturePaths[i];
			
			ifstream test(loc);
			if(!test.is_open())
			{
				cerr<<"Texture file could not be found: "<<loc<<endl;
				//exit(EXIT_FAILURE);
			}
			test.close();
			
			glActiveTexture(GL_TEXTURE0);
			glGenTextures( 1, &texture );
			glBindTexture( GL_TEXTURE_2D, texture );
			image = SOIL_load_image( loc.c_str(), &width, &height, 0, SOIL_LOAD_RGB );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image );
			SOIL_free_image_data( image );
			
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
 
			if(texture == 0)
			{
				cerr<<"Texture failed to load: "<<pmxInfo->texturePaths[i]<<endl;
				printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
				exit(EXIT_FAILURE);
			}
			
			textures.push_back(texture);
			
			cout<<"done"<<endl;
		}
	}
	
	for(int i=1; i<=10; ++i)
	{
		//cout<<"Loading toon"<<i<<".bmp...";
		
		GLuint texture;
		int width, height, channels;
		unsigned char* image;
		stringstream loc;
		if(i!=10) loc<<"data/share/toon0"<<i<<".bmp";
		else loc<<"data/share/toon10.bmp";
		
		ifstream test(loc.str());
		if(!test.is_open())
		{
			//cerr<<"Texture file could not be found: "<<loc.str()<<endl;
			//exit(EXIT_FAILURE);
			
			loc.str(std::string()); //clear ss
			if(i!=10) loc<<DATA_PATH<<"/textures/toon0"<<i<<".bmp";
			else loc<<DATA_PATH<<"/textures/toon10.bmp";
			
			ifstream test2(loc.str());
			if(!test2.is_open())
			{
				cerr<<"Texture file could not be found: "<<loc.str()<<endl;
			}
			
		}
		test.close();
		
		glActiveTexture( GL_TEXTURE0 );
		glGenTextures( 1, &texture );
		glBindTexture( GL_TEXTURE_2D, texture );
			image = SOIL_load_image( loc.str().c_str(), &width, &height, &channels, SOIL_LOAD_RGBA );
		
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image );
		SOIL_free_image_data( image );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		
		if(texture==0)
		{
			cerr<<"Toon Texture failed to load: "<<i<<endl;
			printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
			exit(EXIT_FAILURE);
		}
		
		//cout<<"done"<<endl;
		
		textures.push_back(texture);
	}
	
	
	//FreeImage_DeInitialise();
}


void Viewer::initGLFW()
{
	if (!glfwInit()) exit(EXIT_FAILURE);
	
	glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4); //4x antialiasing
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3); //OpenGL version
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //Don't want old OpenGL
 
	//Open a window and create its OpenGL context
	if( !glfwOpenWindow( 1920, 1080, 0,0,0,0, 32,0, GLFW_WINDOW ) )
	{
		fprintf( stderr, "Failed to open GLFW window\n" );
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	
	cout<<"OpenGL version info: "<<endl;
	cout<<glGetString(GL_VERSION)<<endl;

	// Initialize GLEW
	glewExperimental=true; //Needed in core profile
	if(glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Failed to initialize GLEW\n");
		exit(EXIT_FAILURE);
	}
	glfwSetWindowTitle("PMX Viewer");
	//Ensure we can capture the escape key being pressed below
	glfwEnable( GLFW_STICKY_KEYS );
	
	glfwSetMousePosCallback(mouseMotion);
	glfwSetMouseWheelCallback(mouseWheel);
}

void Viewer::initUniformVarLocations()
{
	uniformVars[uAmbient]=glGetUniformLocation(shaderProgram, "ambient");
	uniformVars[uDiffuse]=glGetUniformLocation(shaderProgram, "diffuse");
	uniformVars[uSpecular]=glGetUniformLocation(shaderProgram, "specular");
	
	uniformVars[uShininess]=glGetUniformLocation(shaderProgram, "shininess");
	
	uniformVars[uIsEdge]=glGetUniformLocation(shaderProgram, "isEdge");
	uniformVars[uEdgeColor]=glGetUniformLocation(shaderProgram, "edgeColor");
	uniformVars[uEdgeSize]=glGetUniformLocation(shaderProgram, "edgeSize");
	
	uniformVars[uHalfVector]=glGetUniformLocation(shaderProgram, "halfVector");
	
	uniformVars[uLightDirection]=glGetUniformLocation(shaderProgram, "lightDirection");
	
	uniformVars[uSphereMode]=glGetUniformLocation(shaderProgram, "fSphereMode");
	
	uniformVars[uTextureSampler]=glGetUniformLocationARB(shaderProgram,"textureSampler");
	uniformVars[uSphereSampler]=glGetUniformLocationARB(shaderProgram,"sphereSampler");
	uniformVars[uToonSampler]=glGetUniformLocationARB(shaderProgram,"toonSampler");
}

Viewer::~Viewer()
{
	glfwTerminate();
}

void Viewer::handleEvents()
{
	glfwPollEvents();
	if(glfwGetKey(GLFW_KEY_UP)==GLFW_PRESS)
	{
		modelTranslate.y-=0.1;
	}
	else if(glfwGetKey(GLFW_KEY_DOWN)==GLFW_PRESS)
	{
		modelTranslate.y+=0.1;
	}
}

void Viewer::holdModelInBindPose()
{
	for(unsigned i = 0; i<pmxInfo->bone_continuing_datasets; i++)
	{
		PMXBone *b=pmxInfo->bones[i];
		
		if(b->parentBoneIndex!=-1)
		{
			PMXBone *parent = pmxInfo->bones[b->parentBoneIndex];
			b->Local = glm::translate( b->position - parent->position );
		}
		else
		{
			b->Local = glm::translate( b->position );
		}
	
		motionController->skinMatrix[i] = b->calculateGlobalMatrix()*motionController->invBindPose[i];
	}
	motionController->updateBoneMatrix();
}



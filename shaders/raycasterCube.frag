#version 330 core
layout(location = 0) out vec4 vFragColor;	//fragment shader output
smooth in vec3 vUV;				
								
uniform sampler3D	volume;		//����������
uniform vec3		camPos;		//���λ��
uniform vec3		step_size;	//��������
//constants
const int MAX_SAMPLES = 300;	
const vec3 texMin = vec3(0);	//��С��������
const vec3 texMax = vec3(1);	//�����������

void main()
{ 
	vec3 dataPos = vUV;
	vec3 geomDir;
	if(abs(camPos.x)<=0.5&&abs(camPos.y)<=0.5&&abs(camPos.z)<=0.5)
	{
		dataPos=camPos+vec3(0.5);
	}
	geomDir = normalize((vUV-vec3(0.5)) - camPos); 
	vec3 dirStep = geomDir * step_size; 
	bool stop = false; 
	vec4 cumc=vec4(0);
	//�����߷�������ۻ���ɫ�Ͳ�͸����
	for (int i = 0; i < MAX_SAMPLES; i++) {
		dataPos = dataPos + dirStep;		
		stop = dot(sign(dataPos-texMin),sign(texMax-dataPos)) < 3.0;
		if (stop) 
			break;
		//��ȡ��������ɫֵ
		vec4 samplec=texture(volume, dataPos).rgba;
		cumc[0]+=samplec.r*samplec[3]*(1-cumc[3]);
		cumc[1]+=samplec.g*samplec[3]*(1-cumc[3]);
		cumc[2]+=samplec.b*samplec[3]*(1-cumc[3]);
		cumc[3]+=samplec.a*(1-cumc[3]);
		if( cumc[3]>0.99)
			break;
	} 
	vFragColor=cumc.rgba;
	//if(dataPos.x<0.5&&dataPos.y<0.5&&dataPos.z<0.5)
	//vFragColor=vec4(camPos.x,camPos.y,camPos.z,1.0);
}
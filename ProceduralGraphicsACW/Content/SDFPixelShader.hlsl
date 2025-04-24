cbuffer MyConstantBuffer : register(b0)
{
	matrix view;
	matrix projection;
	float4 screenDim;
	float4 _time;
	float4 eye;
	float4 viewDir;
};

cbuffer ModelBuffer : register(b1) {
	matrix model;
	matrix vcoralmodel;
	matrix geomCoralModel;
	matrix fishModel;
}

cbuffer LightBuffer : register(b2) {
	float4 skyColour;
	float4 sunLightColour;
	float4 skyLightColour;
	float4 indLightColour;
	float4 horizonColour;
	float4 sunDirection;
	float4 godRayColour;
}

#define TAU 6.28318530718
#define MAX_ITER 5
#define CAUSTICS_SPEED 0.2
#define GOD_RAY_INTENSITY 0.005
#define CAUSTICS_INTENSITY 0.005
#define CAUSTIC_POWER 10.0

Texture2D seafloorTex : register(t0);
Texture2D coralTex : register(t1);
sampler mSampler : register(s0);

static const int MAX_MARCHING_STEPS = 255;
static const float MIN_DIST = 0.0;
static const float MAX_DIST = 100.0;
static const float EPSILON = 0.0001;
static const int METABALL_COUNT_IN_CORAL = 6;
static const float R = 0.04;

static float4 CORAL[6] = {float4(0.0, -0.15, 0.2,R), float4(0.1, -0.23, 0.2,R),
float4(-0.04, -0.28, 0.2, 0.03), float4(0.06, -0.35, 0.2,0.02),
float4(0.0, -0.43, 0.2,R), float4(0.02, -0.55, 0.2,R)};

static float mat = 0.0;

struct VS_Canvas
{
	float4 Position   : SV_POSITION;
	float2 canvasXY   : TEXCOORD0;
};

float hash(float2 grid) {
	float h = dot(grid, float2 (127.1, 311.7));
	return frac(sin(h) * 43758.5453123);
}

float noise(in float2 p) {
	float2 grid = floor(p);
	float2 f = frac(p);
	float2 uv = f * f * (3.0 - 2.0 * f);
	float n1, n2, n3, n4;
	n1 = hash(grid + float2(0, 0));
	n2 = hash(grid + float2(1, 0));
	n3 = hash(grid + float2(0, 1));
	n4 = hash(grid + float2(1, 1));
	n1 = lerp(n1, n2, uv.x);
	n2 = lerp(n3, n4, uv.x);
	n1 = lerp(n1, n2, uv.y);
	return n1;
}

float softAbs2(float x, float a)
{
	float xx = 2.0 * x / a;
	float abs2 = abs(xx);
	if (abs2 < 2.0)
		abs2 = 0.5 * xx * xx * (1.0 - abs2 / 6) + 2.0 / 3.0;
	return abs2 * a / 2.0;
}

float softMax2(float x, float y, float a)
{
	return 0.5 * (x + y + softAbs2(x - y, a));
}

float softMin2(float x, float y, float a)
{
	return   -0.5 * (-x - y + softAbs2(x - y, a));
}

float sphereSDF(float4 centre) {
	return length(centre.xyz) - centre.w;
}

float mergeSoft(float scene, float obj) {
	return softMin2(scene, obj, 0.2);
}

float smoothMin(float a, float b, float k) {
	float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
	return lerp(b, a, h) - k * h * (1.0 - h);
}

float fBm(in float2 p) {
	float sum = -0.1; //Move entire terrain.
	float amp = 0.9; //Amplitude of noise (Gets weird artifacts if too big)
	for (int i = 0; i < 4; ++i) {
		sum += amp * noise(p);
		amp *= 0.5; // More exponential growth that amp. Causes more "spikes", keep < 1
		p *= 1.0; //1.9 As if it's zooming out. Maybe scaling the entire thing.
	}
	return sum * 0.5 + 0.256;
	//^^ 1 = 0.5 , 2 = 0.15
	// 1 Similar thing to amplitude
	// 2 Similar thing to sum. Moves entire terrain
}

float fBmBase(in float2 p) {
	float sum = 0.001; //0.05 Move entire terrain.
	float amp = 0.03; //0.1Amplitude of noise (Gets weird artifacts if too big)
	for (int i = 0; i < 4; ++i) {
		sum += amp * noise(p);
		amp *= 0.9; // More exponential growth that amp. Causes more "spikes", keep < 1
		p *= 4.0; //1.9 As if it's zooming out. Maybe scaling the entire thing.
	}
	return sum * 0.5 + 0.256;
	//^^ 1 = 0.5 , 2 = 0.15
	// 1 Similar thing to amplitude
	// 2 Similar thing to sum. Moves entire terrain
}

float3 caustic(float2 uv)
{
	float2 p = (fmod(uv * TAU, TAU) - 250.0);
	float time = _time.x * CAUSTICS_SPEED + 23.0;
	float2 i = p;
	float c = 1.0;
	float inten = CAUSTICS_INTENSITY;

	for (int n = 0; n < MAX_ITER; n++)
	{
		float t = time * (1.0 - (3.5 / float(n + 1)));
		i = p + float2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
		c += 1.0 / length(float2(p.x / (sin(i.x + t) / inten), p.y / (cos(i.y + t) / inten)));
	}

	c /= float(MAX_ITER);
	c = 1.17 - pow(c, 1.4);
	float3 colour = pow(abs(c), CAUSTIC_POWER);
	colour = clamp(colour + float3(0.0, 0.35, 0.5), 0.0, 1.0);

	colour = lerp(colour, float3(1.0, 0.2, 0.5), 0.3);

	return colour;
}

float causticX(float x, float power, float gtime) {
	float p = fmod(x * TAU, TAU) - 250.0;
	float time = gtime * CAUSTICS_SPEED + 23.0;
	float i = p;
	float c = 1.0; //1.0
	float intensity = GOD_RAY_INTENSITY;

	for (int n = 0; n < MAX_ITER / 2; ++n) {
		float t = time * (1.0 - (3.5 / float(n + 1)));
		i = p + cos(t - i) + sin(t + i);
		c += 1.0 / length(p / (sin(i + t) / intensity));
	}
	c /= float(MAX_ITER);
	c = 1.17 - pow(c, power);

	return c;
}

float GodRays(float2 uv)
{
	float light = 0.0;

	light += pow(causticX((uv.x + 0.08 * uv.y) / 1.7 + 0.5, 1.8, _time.x * 0.65), 10.0) * 0.05;
	light -= pow((1.0 - uv.y) * 0.3, 2.0) * 0.2;
	light += pow(causticX(sin(uv.x), 0.3, _time.x * 0.7), 9.0) * 0.4;
	light += pow(causticX(cos(uv.x * 2.3), 0.3, _time.x * 1.3), 4.0) * 0.1;

	light -= pow((1.0 - uv.y) * 0.3, 3.0);
	light = clamp(light, 0.0, 1.0);

	return light;
}

float createCoral(float3 samplePoint, float3 translate) {
	float res = 1;
	float2 sinTime = float2(sin(_time.x), cos(_time.x));
	float offset = fBmBase(samplePoint.xy);
	for (int i = 0; i < METABALL_COUNT_IN_CORAL; ++i) {
		//float3 p = samplePoint + metaballs[i].xyz + float3(-0.2,-0.8,0.5);
		float3 swayVec = float3(i * 0.05 * sinTime.x, i * 0.02 * sinTime.y, 0) * 0.1;
		float3 p = samplePoint + CORAL[i].xyz + translate + swayVec;
		p.xy += offset * 0.1;
		res = softMin2(res, sphereSDF(float4(p, CORAL[i].w)), 0.17);
	}
	return res;
}

float sceneSDF(float3 samplePoint) {
	mat = 1.0;
	float lifespan = 3.5;
	float bubbleMoveScale = 0.12;
	float buoyancy = 2.0;
	float3 bubbleOffset = float3(-0.8, -0.3, 0.2);

	float3 bubblePos = float3(0,-fmod(_time.y, lifespan), 0) + bubbleOffset;
	float bubbleRadius = 0.1;

	float res = sphereSDF(float4(samplePoint + bubblePos, bubbleRadius));
	//Bubbles definition
	bubblePos = float3(bubbleMoveScale * sin(_time.x * 0.35),-fmod(_time.y * 0.67 * buoyancy, lifespan),0) + bubbleOffset;
	float sp2 = sphereSDF(float4(samplePoint + bubblePos, 0.07));

	bubblePos = float3(bubbleMoveScale * sin(_time.x * 0.65), -fmod(_time.y * 0.58 * buoyancy, lifespan), 0) + bubbleOffset;
	float sp3 = sphereSDF(float4(samplePoint + bubblePos, 0.06));

	bubblePos = float3(bubbleMoveScale * sin(_time.x * 0.5), -fmod(_time.y * 1.12 * buoyancy, lifespan), 0) + bubbleOffset;
	float sp4 = sphereSDF(float4(samplePoint + bubblePos, 0.05));

	bubblePos = float3(bubbleMoveScale * sin(_time.x * 0.5), -fmod(_time.y * 1.35 * buoyancy, lifespan), 0) + bubbleOffset;
	float sp5 = sphereSDF(float4(samplePoint + bubblePos, 0.04));
	
	//Scene construction
	res = mergeSoft(res, sp2);
	res = mergeSoft(res, sp3);
	res = mergeSoft(res, sp4);
	res = mergeSoft(res, sp5);

	//floor 
	float fbBase = fBmBase(samplePoint.xz);
	float fbMain = fBm(samplePoint.xz);
	float fbTot = fbBase + fbMain;

	//coral
	float3 c1 = float3(0.7, -0.8, -0.4);
	float coral = createCoral(samplePoint, c1);
	float3 c2 = float3(0.8, -0.83, -0.6);
	coral = min(coral, createCoral(samplePoint, c2));
	res = min(res, coral);

	if (samplePoint.y - fbTot < EPSILON) {
		mat = 0.0;
	}
	else if (coral < EPSILON) {
		mat = 2.0;
	}
	//
	res = min(res,samplePoint.y - fbTot);

	return res;
}

float shortestDistanceToSurface(float3 eye, float3 marchingDirection, float start, float end) {
	float depth = start;
	for (int i = 0; i < MAX_MARCHING_STEPS; ++i) {
		float dist = sceneSDF(eye + depth * marchingDirection);
		if (dist < EPSILON) {
			return depth;
		}
		depth += dist;
		if (depth >= end) {
			return end;
		}
	}
	return end;
}

float3 estimateNormal(float3 p) {
	return normalize(float3(
		sceneSDF(float3(p.x + EPSILON, p.y, p.z)) - sceneSDF(float3(p.x - EPSILON, p.y, p.z)),
		sceneSDF(float3(p.x, p.y + EPSILON, p.z)) - sceneSDF(float3(p.x, p.y - EPSILON, p.z)),
		sceneSDF(float3(p.x, p.y, p.z + EPSILON)) - sceneSDF(float3(p.x, p.y, p.z - EPSILON))
		));
}

float3 getTerrainNormal(in float3 p) {
	float eps = 0.025;

	float fBx = (fBm(float2(p.x - eps, p.z)) + fBmBase(float2(p.x - eps, p.z))) -
		(fBm(float2(p.x + eps, p.z)) + fBmBase(float2(p.x + eps, p.z)));
	float fBy = (2.0 * eps);
	float fBz = (fBm(float2(p.x, p.z - eps)) + fBmBase(float2(p.x, p.z - eps))) -
		(fBm(float2(p.x, p.z + eps)) + fBmBase(float2(p.x, p.z - eps)));

	return normalize(float3(fBx, fBy, fBz));
}

float3 phongContribForLight(float3 k_d, float3 k_s, float alpha, float3 p, float3 eye, 
	float3 lightPos, float3 lightIntensity) 
{
	float3 N = estimateNormal(p);
	float3 L = normalize(lightPos - p);
	float3 V = normalize(eye - p);
	float3 R = normalize(reflect(-L, N));

	float dotLN = dot(L, N);
	float dotRV = dot(R, V);

	if (dotLN < 0.0) {
		// Light not visible from this point on the surface
		return float3(0.0, 0.0, 0.0);
	}

	if (dotRV < 0.0) {
		// Light reflection in opposite direction as viewer, apply only diffuse
		// component
		return lightIntensity * (k_d * dotLN);
	}
	return lightIntensity * (k_d * dotLN + k_s * pow(dotRV, alpha));
} 

float3 phongIllumination(float3 k_a, float3 k_d, float3 k_s, float alpha, float3 p, float3 eye) {
	const float3 ambientLight = 0.5 * float3(1.0, 1.0, 1.0);
	float3 color = ambientLight * k_a;

	float3 light1Pos = float3(0,
		1.0,
		5.0);
	float3 light1Intensity = float3(0.4, 0.4, 0.4);

	color += phongContribForLight(k_d, k_s, alpha, p, eye,
		light1Pos,
		light1Intensity);
	return color;
}

float4 main(VS_Canvas In) : SV_Target
{
	float2 p = In.canvasXY;

	float3 rayOrigin = eye;

	float3 upDir = normalize(cross(float3(0.0, 1.0, 0.0), viewDir));
	float3 rightDir = normalize(cross(viewDir, upDir));
	float3x3 cam = float3x3(upDir, rightDir, viewDir.xyz);

	float3 npr = normalize(float3(p.xy, 1.0));
	float3 rayDirection = mul(npr, cam);

	// background
	float3 colour = skyColour;
	float sky = 0.0;

	// terrain marching

	float dist = shortestDistanceToSurface(rayOrigin, rayDirection, MIN_DIST, MAX_DIST);

	//If the ray hits something in the scene i.e. terrain
	if (dist >= MAX_DIST) {
		sky = clamp(0.8 * (1.0 - 0.8 * rayDirection.y), 0, 1);
		colour = sky * skyColour;
		colour += ((0.3 * caustic(float2(p.x, p.y * 1.0))) + (0.3 * caustic(float2(p.x, p.y * 2.7)))) * pow(p.y, 4.0);
		colour = lerp(colour, horizonColour, pow(1.0 - pow(rayDirection.y, 4.0), 20.0));
		colour += GodRays(p) * lerp(skyColour.x, 1.0, p.y * p.y) * godRayColour;
		float3 gamma = (float3)0.57;
		return float4(pow(colour, gamma), 1);
	}

	float3 pos = rayOrigin + dist * rayDirection;
	if (mat == 0.0) {
		float3 normal;
		normal = getTerrainNormal(pos);
		float sun = clamp(dot(sunDirection, normal), 0, 1);
		sky = clamp(0.5 + 0.5 * normal.y, 0.0, 1.0);
		float3 seafloorRGB = seafloorTex.Sample(mSampler, float2(pos.x * pow(pos.y, 0.01), pos.z * pow(pos.y, 0.01))).xyz;
		float3 lightDir = sunDirection;
		float diff = max(dot(normal, lightDir), 0.0);
		float3 diffuse = diff * seafloorRGB;
		diffuse *= caustic(float2(lerp(pos.x, pos.y, 0.2), lerp(pos.z, pos.y, 0.2)) * 1.1);
		float3 lightCol = 1.0 * sunLightColour;
		lightCol += 0.7 * sky * skyLightColour;
		colour *= 0.8 * diffuse * lightCol;
		colour = lerp(colour, horizonColour, 1.0 - exp(-0.3 * pow(dist, 1.0)));
		float3 godRayCol = float3(0.5, 0.7, 1.0);
		colour += GodRays(p) * lerp(skyColour.x, 1.0, p.y * p.y) * godRayColour;
		float3 gamma = (float3)0.57;
		return float4(pow(colour, gamma), 1);
	}

	float3 K_a = float3(0.0, 0.1, 0.1);
	float3 K_d = sunLightColour;
	float3 K_s = float3(0.01, 0.01, 0.01);
	float shininess = 1.0;

	if (mat == 2.0) {
		K_d = coralTex.Sample(mSampler, p).rgb + float3(-0.25,0.5,-0.1);

		float3 rgb = K_d;
		float3 N = estimateNormal(pos);
		float sun = clamp(dot(sunDirection, N), 0, 1);
		float3 lightDir = sunDirection;
		float diff = max(dot(N, lightDir), 0.0);
		float3 diffuse = diff * rgb;
		diffuse *= caustic(float2(lerp(pos.x, pos.y, 0.2), lerp(pos.z, pos.y, 0.2)) * 1.1);
		float3 lightCol = 1.0 * sunLightColour;
		lightCol += 0.7 * sky * skyLightColour;
		colour *= 0.8 * diffuse * lightCol;
		colour = lerp(colour, horizonColour, 1.0 - exp(-0.3 * pow(1.0, 1.0)));
		float3 gamma = (float3).57;
		return float4(pow(colour, gamma), 1);
	}

	//reflect
	K_s = float3(0.6, 0.7, 0.5);
	float3 color = (float3)0;
	float3 reflectDir = viewDir;
	float3 reflectCol = float3(0, 0, 0);
	float reflectDist = 0.0;
	float3 normal2 = estimateNormal(pos);

	float3 reflectNormal = normal2;
	float3 pReflect = pos;

	for (int i = 0; i < 3; ++i) {
		reflectDir = reflect(reflectDir, reflectNormal);
		reflectDist = shortestDistanceToSurface(pReflect + reflectNormal * 0.05, reflectDir, MIN_DIST, MAX_DIST);
		if (reflectDist > MAX_DIST - EPSILON) {
			break;
		}
		pReflect = pReflect + reflectDist * reflectDir;
		reflectCol = phongIllumination(K_a, K_d, K_s, shininess, pReflect, eye);
		reflectCol *= lerp(reflectCol, float3(0.5, 0.5, 0.5), saturate(reflectDist * 0.05));
		reflectCol *= 2;
		reflectNormal = estimateNormal(pReflect);
	}
	color += reflectCol;

	float alpha = saturate(1.0 - (colour.r + colour.g + colour.b) / 3.0);

	return float4(color, 1-alpha);

}
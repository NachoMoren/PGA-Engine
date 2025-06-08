///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

	#if defined(VERTEX) ///////////////////////////////////////////////////

	layout(location = 0) in vec3 aPosition;
	layout(location = 1) in vec2 aTexCoord;

	out vec2 vTexCoord;

	void main()
	{
		vTexCoord = aTexCoord;
		gl_Position = vec4(aPosition, 1.0);
	}

	#elif defined(FRAGMENT) ///////////////////////////////////////////////

	in vec2 vTexCoord;

	uniform sampler2D uTexture;

	layout(location = 0) out vec4 oColor;

	void main()
	{
		oColor = texture(uTexture, vTexCoord);
	}

	#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef GEOMETRY_RENDER

	#if defined(VERTEX) ///////////////////////////////////////////////////
	layout(location = 0) in vec3 aPosition;
	layout(location = 1) in vec3 aNormal;
	layout(location = 2) in vec2 aTexCoord;

	layout(binding = 1, std140) uniform LocalParams
	{
		mat4 uWorldMatrix;
		mat4 uWorldViewProjectionMatrix;
	};

	layout(binding = 2, std140) uniform ClippingPlane
	{
		vec4 clippingPlane;
	};

	out vec2 vTexCoord;
	out vec3 vPosition;
	out vec3 vNormal;

	void main()
	{
		vTexCoord = aTexCoord;
		vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
		vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
		
		gl_ClipDistance[0] = dot(vec4(vPosition, 0.0), clippingPlane);

		gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
	}

	#elif defined(FRAGMENT) ///////////////////////////////////////////////

	in vec2 vTexCoord;
	in vec3 vPosition;
	in vec3 vNormal;

	uniform sampler2D uTexture;
	uniform float uNear; 
	uniform float uFar;	

	layout(location = 0) out vec4 oColor;
	layout(location = 1) out vec4 oPosition;
	layout(location = 2) out vec4 oNormal;
	layout(location = 3) out vec4 oDepth;

	float LinearizeDepth(float depth)
	{
		float z = depth * 2.0 - 1.0; 
		return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
	}

	void main()
	{
		vec4 texColor = texture(uTexture, vTexCoord);
		if(texColor.a < 0.1)
			discard;
		oColor = texColor;
		//oColor = texture(uTexture, vTexCoord);
		oPosition = vec4(vPosition, 1.0);
		oNormal = vec4(normalize(vNormal), 1.0);


		float depth = LinearizeDepth(gl_FragCoord.z) / uFar;
		oDepth = vec4(vec3(depth),1.0);
	}

	#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifdef LIGHTING_RENDER
	#if defined(VERTEX) ///////////////////////////////////////////////////

	layout(location = 0) in vec3 aPosition;
	layout(location = 1) in vec2 aTexCoord;

	struct Light
	{
		uint type;
		vec3 color;
		vec3 direction;
		vec3 position;
	};

	layout(binding = 0, std140) uniform GlobalParams
	{
		vec3 uCameraPosition;
		uint uLightCount;
		Light uLights[16];
	};

	out vec2 vTexCoord;

	void main()
	{
		vTexCoord = aTexCoord;
		gl_Position = vec4(aPosition, 1.0);
	}

	#elif defined(FRAGMENT) ///////////////////////////////////////////////

	in vec2 vTexCoord;

	uniform sampler2D uAlbedo;
	uniform sampler2D uPosition;
	uniform sampler2D uNormal;

	struct Light
	{
		uint type;
		vec3 color;
		vec3 direction;
		vec3 position;
	};

	layout(location = 0) out vec4 oColor;
	layout(location = 1) out vec4 oBloom;

	layout(binding = 0, std140) uniform GlobalParams
	{
		vec3 uCameraPosition;
		uint uLightCount;
		Light uLights[16];
	};

	vec3 CalculateDirLight(Light light, vec3 normal, vec3 viewDir)
	{
		vec3 lightDir = normalize(-light.direction);
	
		float ambientStrength = 0.2f;
		vec3 ambient = ambientStrength * light.color;

		float diff = max(dot(normal, lightDir), 0.0);
		vec3 diffuse = diff * light.color;

		float specularStrength = 0.1f;
		vec3 reflectDir = reflect(-lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
		vec3 specular = specularStrength * spec * light.color;

		return ambient + diffuse + specular;
	}

	vec3 CalculatePointLight(Light light, vec3 normal, vec3 pos, vec3 viewDir)
	{

		vec3 lightDir = normalize(light.position - pos);
		float constant = 1.0f;
		float linear = 0.09f;
		float quadratic = 0.032f;
		float distance = length(light.position - pos);
		float attenuation = 1.0f / (constant + linear * distance + quadratic * (distance * distance));

		float ambientStrength = 0.2f;
		vec3 ambient = ambientStrength * light.color;

		float diff = max(dot(normal, lightDir), 0.0);
		vec3 diffuse = diff * light.color;

		float specularStrength = 0.1f;
		vec3 reflectDir = reflect(-lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
		vec3 specular = specularStrength * spec * light.color;
		
		return (ambient + diffuse + specular) * attenuation;
	}

	void main()
	{
		vec3 albedo = texture(uAlbedo, vTexCoord).rgb;
		vec3 fragPos = texture(uPosition, vTexCoord).rgb;
		vec3 normal = texture(uNormal, vTexCoord).rgb;
		float alpha = texture(uAlbedo, vTexCoord).a;

		vec3 viewDir = normalize(uCameraPosition - fragPos);
		vec3 lightColor = vec3(0.0);

		for(int i = 0; i < uLightCount; i++)
		{
			Light light = uLights[i];
			if (light.type == 0)
			{
				lightColor += CalculateDirLight(light, normal, viewDir) * albedo;
			}
			else if (light.type == 1)
			{
				vec3 lightDir = normalize(light.position - fragPos);
				float distance = length(lightDir);
				lightColor += CalculatePointLight(light, normal, fragPos, viewDir) * albedo;
			}
			
		}
		oColor = vec4(lightColor, 1.0);
		oBloom = oColor;
		
	}

	#endif
#endif

#ifdef BLOOM

	#if defined(VERTEX) ///////////////////////////////////////////////////

	layout(location = 0) in vec3 aPosition;
	layout(location = 1) in vec2 aTexCoord;

	out vec2 vTexCoord;

	void main()
	{
		vTexCoord = aTexCoord;
		gl_Position = vec4(aPosition, 1.0);
	}

	#endif
	#if defined(FRAGMENT) ///////////////////////////////////////////////////

		out vec4 FragColor;
  
		in vec2 vTexCoord;

		uniform sampler2D uMainTexture;
		uniform sampler2D uColorMap;
		uniform int uMaxLod;

		void main()
		{    
			vec3 color = texture(uMainTexture, vTexCoord).rgb;
			vec3 bloomColor = vec3(0.0);
			for (int lod = 0; lod < uMaxLod; ++lod) {
				bloomColor += textureLod(uColorMap, vTexCoord, lod).rgb;
			}
			bloomColor /= float(uMaxLod); // optional: average the bloom

			color += bloomColor;
			FragColor = vec4(color, 1.0);
		}

	#endif
#endif

#ifdef BLUR

	#if defined(VERTEX) ///////////////////////////////////////////////////

	layout(location = 0) in vec3 aPosition;
	layout(location = 1) in vec2 aTexCoord;

	out vec2 vTexCoord;

	void main()
	{
		vTexCoord = aTexCoord;
		gl_Position = vec4(aPosition, 1.0);
	}

	#endif
	#if defined(FRAGMENT) ///////////////////////////////////////////////////

		out vec4 FragColor;
  
		in vec2 vTexCoord;

		uniform sampler2D uColorMap;
		uniform vec2 uDir; 
		uniform int uInputLod;
		uniform int kernelRadius; 
		uniform float uLodIntensity;

		void main()
		{             
			vec2 texSize = textureSize(uColorMap, uInputLod);
			vec2 texelSize = 1.0 / texSize; // gets size of single texel
			vec2 margin1 = texelSize * 0.5;
			vec2 margin2 = vec2(1.0) - margin1;

			FragColor = vec4(0.0);

			vec2 directionFragCoord = gl_FragCoord.xy * uDir;
			int coord = int(directionFragCoord.x + directionFragCoord.y);
			vec2 directionTexSize = texelSize * uDir;
			int size = int(directionTexSize.x + directionTexSize.y);
			float weight = 0.0;
			FragColor = vec4(0.0);

			for (int i = -kernelRadius; i <= kernelRadius; i++) {
				float w = smoothstep(0.0, float(kernelRadius), float(kernelRadius - abs(i)));
				vec2 offset = uDir * texelSize * float(i);
				vec2 sampleCoord = clamp(vTexCoord + offset, margin1, margin2);
				FragColor += textureLod(uColorMap, sampleCoord, uInputLod) * w * uLodIntensity;
				weight += w;
			}

			FragColor /= weight;
		}

	#endif
#endif


#ifdef SHOW_LIGHTS

	#if defined(VERTEX) ///////////////////////////////////////////////////

	layout(location = 0) in vec3 aPosition;

	uniform mat4 uProjectionMatrix;

	void main()
	{
		gl_Position = uProjectionMatrix * vec4(aPosition, 1.0);
	}	

	#elif defined(FRAGMENT) ///////////////////////////////////////////////

	layout(location = 0) out vec4 oColor;
	uniform vec3 uLightColor;

	void main()
	{
		oColor = vec4(uLightColor, 1.0);
	}
	#endif

#endif
	#ifdef WATER_EFFECT

	#if defined(VERTEX) ///////////////////////////////////////////////////

	layout (location = 0) in vec3 aPosition;
	layout (location = 1) in vec3 aNormal;

	uniform mat4 uView;
	uniform mat4 uProjection;

	out vec4 clipSpace; 
	out vec2 vTexCoord;

	const float tiling = 1.0; 

	void main()
	{
		clipSpace = uProjection * uView * vec4(aPosition, 1.0);
		gl_Position = clipSpace;
		vTexCoord = vec2(aPosition.x/2.0 + 0.5, aPosition.z/2.0 + 0.5) * tiling;

	}

	#elif defined(FRAGMENT) ///////////////////////////////////////////////

	
	uniform vec2 uViewportSize;
	uniform mat4 uViewInverse; 
	uniform mat4 uProjectionInverse;

	uniform sampler2D uReflectionMap;
	uniform sampler2D uRefractionMap;
	uniform sampler2D uReflectionDepth;
	uniform sampler2D uRefractionDepth;
	uniform sampler2D uNormalMap;
	uniform sampler2D uDudvMap;

	uniform float moveFactor;

	in vec4 clipSpace;
	in vec2 vTexCoord;

	const float waveStrength = 0.01; // Adjust wave strength
	out vec4 oColor;

	

	void main()
	{
		vec2 ndc = (clipSpace.xy / clipSpace.w)/2.0 + 0.5;
		vec2 refractTexCoords = vec2(ndc.x, ndc.y);
		vec2 reflectTexCoords = vec2(ndc.x,  - ndc.y);

		vec2 distortion = (texture(uDudvMap, mod(vec2(vTexCoord.x + moveFactor, vTexCoord.y), 1.0)).rg * 2.0 - 1.0) * waveStrength;
		vec2 distortion2 = (texture(uDudvMap, mod(vec2(vTexCoord.x + moveFactor, vTexCoord.y + moveFactor), 1.0)).rg * 2.0 - 1.0) * waveStrength;
		vec2 totalDistortion = distortion + distortion2;
		refractTexCoords += totalDistortion;
		refractTexCoords = clamp(refractTexCoords, 0.001, 0.999);
		reflectTexCoords += totalDistortion;
		reflectTexCoords.x = clamp(reflectTexCoords.x, 0.001, 0.999);
		reflectTexCoords.y = clamp(reflectTexCoords.y, -0.999, 0.001);

		vec4 refractColor = texture(uRefractionMap, refractTexCoords);
		vec4 reflectColor = texture(uReflectionMap, reflectTexCoords);

		oColor = mix(refractColor, reflectColor, 0.5);
		oColor = mix(oColor, vec4(0.0, 0.3, 0.5, 1.0), 0.2); 
	}

	#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.

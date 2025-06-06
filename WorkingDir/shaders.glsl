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

	out vec2 vTexCoord;
	out vec3 vPosition;
	out vec3 vNormal;

	void main()
	{
		vTexCoord = aTexCoord;
		vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
		vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
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
		oColor = texture(uTexture, vTexCoord);
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
#ifdef SHOW_WATER

	#if defined(VERTEX) ///////////////////////////////////////////////////

	layout (location = 0) in vec3 aPosition;
	layout (location = 1) in vec2 aNormal;

	uniform mat4 uView;
	uniform mat4 uProjection;

	out Data
	{
		vec3 positionViewspace;
		vec3 normalViewspace;
	} VSOut; 

	void main()
	{
		VSOut.positionViewspace = vec3(uView * vec4(aPosition, 1.0));
		VSOut.normalViewspace = vec3(uView * vec4(aNormal, 0.0));
		gl_Position = uProjection * vec4(VSOut.positionViewspace, 1.0);
	}

	#elif defined(FRAGMENT) ///////////////////////////////////////////////

	uniform vec2 viewportSize; 
	uniform mat4 uView;
	uniform mat4 uViewInverse; 
	uniform mat4 uProjection;
	uniform sampler2D reflectionMap; 
	uniform sampler2D refractionMap;
	uniform sampler2D reflectionDepth;
	uniform sampler2D refractionDepth;
	uniform sampler2D normalMap;
	uniform sampler2D dudvMap;

	in Data
	{
		vec3 positionViewspace;
		vec3 normalViewspace;
	} FSIn;

	out vec4 oColor;

	vec3 fresnelSchlick(vec3 F0, float cosTheta)
	{
		return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	}

	vec3 reconstructPixelPosition(float depth) 
	{
		vec2 texCoord = gl_FragCoord.xy / viewportSize;
		vec3 positionNDC = vec3(texCoords * 2.0 - 1.0, depth * 2.0 - 1.0);
		vec4 positionEyespace = uProjection * vec4(positionNDC, 1.0);
		positionEyespace.xyz /= positionEyespace.w; 
		return positionEyespace.xyz;
	}

	void main()
	{
		vec3 N = normalize(FSIn.normalViewspace);
		vec3 V = normalize(-FSIn.positionViewspace);
		vec3 Pw = vec3(uViewInverse * vec4(FSIn.positionViewspace, 1.0));
		vec2 texCoord = gl_FragCoord.xy / viewportSize;

		const vec2 waveLength = vec2(2.0);
		const vec2 waveStrength = vec2(0.05);
		const float turbidityDistance = 10.0;

		vec2 distortion = (2.0 * texture(dudvMap, Pw.xz/waveLength).rg - vec2(1.0)) * waveStrength + waveStrength/7.0;
		vec2 reflectionTexCoord = vec2(texCoord.s, 1.0 - texCoord.t) + distortion;
		vec2 refractionTexCoord = texCoord + distortion;
		vec3 reflectionColor = texture(reflectionMap, reflectionTexCoord).rgb;
		vec3 refractionColor = texture(refractionMap, refractionTexCoord).rgb;

		float distortedGroundDepth = texture(refractionDepth, refractionTexCoord).x;
		vec3 distortedGroundPosViewspace = reconstructPixelPosition(distortedGroundDepth);
		float distortedWaterDepth = FSIn.positionViewspace.z - distortedGroundPosViewspace.z;
		float tintFactor = clamp(distortedWaterDepth / turbidityDistance, 0.0, 1.0);
		vec3 waterColor = vec3(0.25, 0.4, 0.6);
		refractionColor = mix(refractionColor, waterColor, tintFactor);

		vec3 F0 = vec3(0.1);
		vec3 F = fresnelSchlick(max(0.0, dot(V,N)), F0);
		oColor.rgb = mix(refractionColor, reflectionColor, F);
		oColor.a = 1.0;
	}

	#endif
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.

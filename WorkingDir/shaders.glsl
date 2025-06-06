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
			const float gamma = 2.2;
			vec3 color = texture(uMainTexture, vTexCoord).rgb;
			vec3 bloomColor = texture(uColorMap, vTexCoord).rgb;
			color += bloomColor;

			//vec3 result = vec3(1.0) - exp(-color * uMaxLod);

			//result = pow(result, vec3(1.0 / gamma)); 
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
			int kernelRadius = 24;
			float weight = 0.0;
			FragColor = vec4(0.0);

			for (int i = -kernelRadius; i <= kernelRadius; i++) {
				float w = smoothstep(0.0, float(kernelRadius), float(kernelRadius - abs(i)));
				vec2 offset = uDir * texelSize * float(i);
				vec2 sampleCoord = clamp(vTexCoord + offset, margin1, margin2);
				FragColor += textureLod(uColorMap, sampleCoord, uInputLod) * w;
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

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.

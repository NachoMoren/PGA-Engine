#ifdef RENDER_GEOMETRY

	#if defined(VERTEX) ///////////////////////////////////////////////////

	layout(location=0) in vec3 aPosition;
	layout(location=1) in vec3 aNormal;
	layout(location=2) in vec2 aTexCoord;
	//layout(location=3) in vec3 aTangent;
	//layout(location=4) in vec3 aBitangent;

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

	layout(binding = 1, std140) uniform LocalParams
	{
		mat4 uWorldMatrix;
		mat4 uWorldViewProjectionMatrix;
	};

	out vec2 vTexCoord;
	out vec3 vPosition;
	out vec3 vNormal;
	out vec3 vViewDir;

	void main()
	{
		vTexCoord = aTexCoord;
		vPosition = vec3(uWorldMatrix * vec4(aPosition,1.0));
		vNormal = vec3(uWorldMatrix * vec4(aNormal,0.0));
		vViewDir = uCameraPosition - vPosition;
		float clippingScale = 1.0f;

		gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, clippingScale);
	}

	#elif defined(FRAGMENT) ///////////////////////////////////////////////

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

	in vec2 vTexCoord;
	in vec3 vPosition; 
	in vec3 vNormal;  
	in vec3 vViewDir;

	uniform sampler2D uTexture;

	uniform int uViewmode;

	layout(location=0) out vec4 oColor;
	layout(location=1) out vec4 oPosition;
	layout(location=2) out vec4 oNormal;
	layout(location=3) out vec4 oDepth;

	float near = 0.1; 
	float far  = 1000.0; 

	float LinearizeDepth(float depth) 
	{
		float z = depth * 2.0 - 1.0; 
		return (2.0 * near * far) / (far + near - z * (far - near));	
	}

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
		vec3 texColor = texture(uTexture, vTexCoord).rgb;
		vec3 normal = normalize(vNormal);
		vec3 viewDir = normalize(vViewDir);

		vec3 finalColor = vec3(0.0);

		oColor = vec4(texColor, 1.0);
		oPosition = vec4(vPosition, 1.0);
		oNormal = vec4(normal, 1.0);
		oDepth = vec4(LinearizeDepth(gl_FragCoord.z) / far, 0.0, 0.0, 1.0);

		for(int i = 0; i<uLightCount; i++){
			if(uLights[i].type == 0){

				Light light = uLights[i];
				finalColor += CalculateDirLight(light, normal, viewDir) * texColor;
			}
			else
			{
				Light light = uLights[i];
				finalColor += CalculatePointLight(light, normal, vPosition, viewDir) * texColor;
			}
		}
		oColor = vec4(finalColor, 1.0);
	}

	#endif
#endif

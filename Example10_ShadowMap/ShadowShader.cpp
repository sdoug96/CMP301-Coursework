// texture shader.cpp
#include "shadowshader.h"

ShadowShader::ShadowShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"shadow_vs.cso", L"shadow_ps.cso");
}

ShadowShader::~ShadowShader()
{
	if (sampleState)
	{
		sampleState->Release();
		sampleState = 0;
	}
	if (matrixBuffer)
	{
		matrixBuffer->Release();
		matrixBuffer = 0;
	}
	if (layout)
	{
		layout->Release();
		layout = 0;
	}
	if (lightBuffer)
	{	
		lightBuffer->Release();
		lightBuffer = 0;
	}
	// Release the fog constant buffer.
	if (fogBuffer)
	{
		fogBuffer->Release();
		fogBuffer = 0;
	}

	// Release the second fog constant buffer.
	if (fogBuffer1)
	{
		fogBuffer1->Release();
		fogBuffer1 = 0;
	}

	// Release the camera constant buffer.
	if (cameraBuffer)
	{
		cameraBuffer->Release();
		cameraBuffer = 0;
	}

	//Release base shader components
	BaseShader::~BaseShader();
}

void ShadowShader::initShader(WCHAR* vsFilename, WCHAR* psFilename)
{
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC lightBufferDesc;
	D3D11_BUFFER_DESC fogBufferDesc;
	D3D11_BUFFER_DESC fogBufferDesc1;
	D3D11_BUFFER_DESC cameraBufferDesc;

	// Load (+ compile) shader files
	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);

	// Sampler for shadow map sampling.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	renderer->CreateSamplerState(&samplerDesc, &sampleStateShadow);

	// Setup light buffer
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightBufferType);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&lightBufferDesc, NULL, &lightBuffer);

	// Setup the description of the dynamic fog constant buffer that is in the vertex shader.
	fogBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	fogBufferDesc.ByteWidth = sizeof(FogBufferType);
	fogBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	fogBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	fogBufferDesc.MiscFlags = 0;
	fogBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	renderer->CreateBuffer(&fogBufferDesc, NULL, &fogBuffer);

	// Setup the description of the second dynamic fog constant buffer that is in the pixel shader.
	fogBufferDesc1.Usage = D3D11_USAGE_DYNAMIC;
	fogBufferDesc1.ByteWidth = sizeof(FogBufferType1);
	fogBufferDesc1.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	fogBufferDesc1.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	fogBufferDesc1.MiscFlags = 0;
	fogBufferDesc1.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	renderer->CreateBuffer(&fogBufferDesc1, NULL, &fogBuffer1);

	//Setup camera buffer
	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof(CameraBufferType);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraBufferDesc.MiscFlags = 0;
	cameraBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&cameraBufferDesc, NULL, &cameraBuffer);
}

void ShadowShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX &worldMatrix, const XMMATRIX &viewMatrix, const XMMATRIX &projectionMatrix, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* heightMap, ID3D11ShaderResourceView*depthMap, ID3D11ShaderResourceView*depthMap1, ID3D11ShaderResourceView*depthMap2, ID3D11ShaderResourceView*depthMap3, Light* light, Light* light1, Light* light2, Light* light3, XMFLOAT4 lightDir, XMFLOAT4 light1Dir, XMFLOAT4 light2Dir, XMFLOAT4 light3Dir, float fogStart, float fogEnd, Camera* cam, bool fogDisable)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	LightBufferType* lightPtr;
	FogBufferType* fogPtr;
	FogBufferType1* fogPtr1;
	CameraBufferType* camPtr;
	
	// Transpose the matrices to prepare them for the shader.
	XMMATRIX tworld = XMMatrixTranspose(worldMatrix);
	XMMATRIX tview = XMMatrixTranspose(viewMatrix);
	XMMATRIX tproj = XMMatrixTranspose(projectionMatrix);

	XMMATRIX tLightViewMatrix = XMMatrixTranspose(light->getViewMatrix());
	XMMATRIX tLightProjectionMatrix = XMMatrixTranspose(light->getOrthoMatrix());

	XMMATRIX tLightViewMatrix1 = XMMatrixTranspose(light1->getViewMatrix());
	XMMATRIX tLightProjectionMatrix1 = XMMatrixTranspose(light1->getOrthoMatrix());

	XMMATRIX tLightViewMatrix2 = XMMatrixTranspose(light2->getViewMatrix());
	XMMATRIX tLightProjectionMatrix2 = XMMatrixTranspose(light2->getOrthoMatrix());

	XMMATRIX tLightViewMatrix3 = XMMatrixTranspose(light3->getViewMatrix());
	XMMATRIX tLightProjectionMatrix3 = XMMatrixTranspose(light3->getOrthoMatrix());
	
	// Lock the constant buffer so it can be written to.
	deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;

	dataPtr->world = tworld;// worldMatrix;
	dataPtr->view = tview;
	dataPtr->projection = tproj;

	dataPtr->lightView = tLightViewMatrix;
	dataPtr->lightProjection = tLightProjectionMatrix;

	dataPtr->lightView1 = tLightViewMatrix1;
	dataPtr->lightProjection1 = tLightProjectionMatrix1;

	dataPtr->lightView2 = tLightViewMatrix2;
	dataPtr->lightProjection2 = tLightProjectionMatrix2;

	dataPtr->lightView3 = tLightViewMatrix3;
	dataPtr->lightProjection3 = tLightProjectionMatrix3;

	deviceContext->Unmap(matrixBuffer, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

	// Send light data to pixel shader
	deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	lightPtr = (LightBufferType*)mappedResource.pData;

	lightPtr->ambient[0] = light->getAmbientColour();
	lightPtr->ambient[1] = light1->getAmbientColour();
	lightPtr->ambient[2] = light2->getAmbientColour();
	lightPtr->ambient[3] = light3->getAmbientColour();

	lightPtr->diffuse[0] = light->getDiffuseColour();
	lightPtr->diffuse[1] = light1->getDiffuseColour();
	lightPtr->diffuse[2] = light2->getDiffuseColour();
	lightPtr->diffuse[3] = light3->getDiffuseColour();

	lightPtr->direction[0] = lightDir;
	lightPtr->direction[1] = light1Dir;
	lightPtr->direction[2] = light2Dir;
	lightPtr->direction[3] = light3Dir;

	deviceContext->Unmap(lightBuffer, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &lightBuffer);

	// Send matrix data
	deviceContext->Map(fogBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	fogPtr = (FogBufferType*)mappedResource.pData;
	fogPtr->fogStart = fogStart;
	fogPtr->fogEnd = fogEnd;
	fogPtr->padding.x = 0;
	fogPtr->padding.y = 0;

	deviceContext->Unmap(fogBuffer, 0);
	deviceContext->VSSetConstantBuffers(1, 1, &fogBuffer);

	// Send matrix data
	deviceContext->Map(fogBuffer1, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	fogPtr1 = (FogBufferType1*)mappedResource.pData;
	fogPtr1->fogDisable = fogDisable;
	fogPtr1->padding.x = 0;
	fogPtr1->padding.y = 0;
	fogPtr1->padding.z = 0;

	deviceContext->Unmap(fogBuffer1, 0);
	deviceContext->PSSetConstantBuffers(1, 1, &fogBuffer1);

	//Send camera data
	deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	camPtr = (CameraBufferType*)mappedResource.pData;
	camPtr->cameraPosition = cam->getPosition();
	camPtr->padding = 0.0f;

	deviceContext->Unmap(cameraBuffer, 0);
	deviceContext->VSSetConstantBuffers(2, 1, &cameraBuffer);

	// Set shader texture resource in the pixel shader.
	deviceContext->PSSetShaderResources(0, 1, &texture);

	deviceContext->PSSetShaderResources(1, 1, &depthMap);
	deviceContext->PSSetShaderResources(2, 1, &depthMap1);
	deviceContext->PSSetShaderResources(3, 1, &depthMap2);
	deviceContext->PSSetShaderResources(4, 1, &depthMap3);

	deviceContext->PSSetSamplers(0, 1, &sampleState);
	deviceContext->PSSetSamplers(1, 1, &sampleStateShadow);

	// Set shader heightMap resource in the vertex shader.
	deviceContext->VSSetShaderResources(0, 1, &heightMap);
	deviceContext->VSSetSamplers(0, 1, &sampleState);
}


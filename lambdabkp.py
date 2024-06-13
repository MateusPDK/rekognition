import json
import boto3
import base64

def generate_response(status_code=200, message="Success", data=None):
    response = {
        'statusCode': status_code,
        'body': json.dumps({
            'message': message,
            'data': data
        })
    }
    return response

def lambda_handler(event, context):
    try:
        # Log the entire event received
        print("Received event: " + json.dumps(event, indent=2))

        rekognition = boto3.client('rekognition')
        
        # Verificar se o corpo está presente
        if 'body' not in event:
            raise ValueError("Missing 'body' in event")
        
        image_data = event['body']
        
        # Log the body received
        print("Received body: " + image_data[:50] + "...")  # Log only the first 50 characters for brevity
        
        # Verificar se a imagem decodificada é válida
        if not image_data:
            raise ValueError("Image data could not be decoded")
        
        # Decoding base64 image data
        try:
            image_bytes = base64.b64decode(image_data)
        except Exception as e:
            raise ValueError("Failed to decode base64 image data: " + str(e))
        
        # Log the decoded image data length
        print(f"Decoded image data length: {len(image_bytes)} bytes")

        response = rekognition.detect_faces(
            Image={'Bytes': image_bytes},
            Attributes=['ALL']
        )
        
        return generate_response(status_code=200, message="Image processed successfully", data=response)
    
    except Exception as e:
        print("Error: " + str(e))
        return generate_response(status_code=500, message=str(e))


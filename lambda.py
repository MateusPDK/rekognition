import json
import boto3
import base64
import uuid
from datetime import datetime

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
        s3 = boto3.client('s3')
        
        # Check if the body is present
        if 'body' not in event:
            raise ValueError("Missing 'body' in event")
        
        image_data = event['body']
        
        # Log the body received
        print("Received body: " + image_data[:50] + "...")  # Log only the first 50 characters for brevity
        
        # Check if the image data is valid
        if not image_data:
            raise ValueError("Image data could not be decoded")
        
        # Decoding base64 image data
        try:
            image_bytes = base64.b64decode(image_data)
        except Exception as e:
            raise ValueError("Failed to decode base64 image data: " + str(e))
        
        # Log the decoded image data length
        print(f"Decoded image data length: {len(image_bytes)} bytes")

        # Save image to S3
        bucket_name = 'esp32rekognition'
        folder_name = 'faces_history/'
        image_id = str(uuid.uuid4())
        image_key = f"{folder_name}{image_id}.png"
        
        s3.put_object(
            Bucket=bucket_name,
            Key=image_key,
            Body=image_bytes,
            ContentType='image/png'
        )
        
        print(f"Image saved to S3: {bucket_name}/{image_key}")

        # Get list of student images from S3
        student_folder = 'students/'
        student_objects = s3.list_objects_v2(Bucket=bucket_name, Prefix=student_folder)
        
        if 'Contents' not in student_objects:
            raise ValueError("No student images found in the bucket")
        
        matched = False
        for student_object in student_objects['Contents']:
            student_image_key = student_object['Key']
            if student_image_key.endswith('/'):
                continue  # Skip folders
            
            student_image_obj = s3.get_object(Bucket=bucket_name, Key=student_image_key)
            student_image_bytes = student_image_obj['Body'].read()
            
            # Compare faces using Rekognition
            response = rekognition.compare_faces(
                SourceImage={'Bytes': image_bytes},
                TargetImage={'Bytes': student_image_bytes},
                SimilarityThreshold=90  # Adjust this threshold as needed
            )
            
            if response['FaceMatches']:
                student_name = student_image_key.split('/')[-1].split('.')[0]
                print(f"Match found: {student_name}")
                log_info = {
                    'MatchedStudent': student_name,
                    'Timestamp': datetime.now().isoformat(),
                    'ImageKey': image_key
                }
                print(f"Log info: {json.dumps(log_info)}")
                return generate_response(status_code=200, message=f"Bem vindo {student_name}!")
        
        # If no matches were found
        log_info = {
            'MatchedStudent': None,
            'Timestamp': datetime.now().isoformat(),
            'ImageKey': image_key
        }
        print(f"Log info: {json.dumps(log_info)}")
        return generate_response(status_code=200, message="Estudante não encontrado!")
    
    except Exception as e:
        print("Error: " + str(e))
        return generate_response(status_code=500, message=str(e))

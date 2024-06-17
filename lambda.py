import json
import boto3
import base64
import uuid
from datetime import datetime

def generate_response(status_code=200, message="Sucesso", data=None):
    response = {
        'statusCode': status_code,
        'body': json.dumps({
            'message': message,
            'data': data
        })
    }
    return response

def send_email(student_name, image):
    ses = boto3.client('ses')
    email_body = f"Estudante {student_name} marcou presença na aula do dia {datetime.now().strftime('%d/%m/%Y')}. Foto: https://esp32rekognition.s3.amazonaws.com/{image}"
    response = ses.send_email(
        Source='mateus.podgorski@sou.unijui.edu.br',
        Destination={
            'ToAddresses': ['mateus-pk@outlook.com']
        },
        Message={
            'Subject': {
                'Data': 'Presença Marcada'
            },
            'Body': {
                'Text': {
                    'Data': email_body
                }
            }
        }
    )
    return response

def lambda_handler(event, context):
    try:
        print("Evento recebido: " + json.dumps(event, indent=2))

        rekognition = boto3.client('rekognition')
        s3 = boto3.client('s3')
        
        if 'body' not in event:
            raise ValueError("Faltando 'body' no evento")
        
        image_data = event['body']
        print("Corpo recebido: " + image_data[:50] + "...")
        
        if not image_data:
            raise ValueError("Os dados da imagem não puderam ser decodificados")
        
        try:
            image_bytes = base64.b64decode(image_data)
        except Exception as e:
            raise ValueError("Falha ao decodificar dados da imagem base64: " + str(e))
        
        print(f"Comprimento dos dados da imagem decodificada: {len(image_bytes)} bytes")

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
        
        print(f"Imagem salva no S3: {bucket_name}/{image_key}")

        student_folder = 'students/'
        student_objects = s3.list_objects_v2(Bucket=bucket_name, Prefix=student_folder)
        print("Lista de estudantes recuperada")
        
        if 'Contents' not in student_objects:
            raise ValueError("Nenhuma imagem de estudante encontrada no bucket")
        
        print(f"Estudantes autorizados: {student_objects['Contents']}")
        matched = False
        for student_object in student_objects['Contents']:
            student_image_key = student_object['Key']
            if student_image_key == student_folder or student_image_key.endswith('/'):
                continue
            
            try:
                student_image_obj = s3.get_object(Bucket=bucket_name, Key=student_image_key)
                student_image_bytes = student_image_obj['Body'].read()
            except Exception as e:
                print(f"Falha ao buscar imagem do estudante {student_image_key}: {str(e)}")
                continue
            
            print(f"Comparando foto com estudante: {student_image_key}")
            try:
                response = rekognition.compare_faces(
                    SourceImage={'Bytes': image_bytes},
                    TargetImage={'Bytes': student_image_bytes},
                    SimilarityThreshold=90
                )
            except Exception as e:
                print(f"Rekognition compare_faces falhou para {student_image_key}: {str(e)}")
                continue
            
            if response['FaceMatches']:
                student_name = student_image_key.split('/')[-1].split('.')[0]
                print(f"Match encontrado: {student_name}")
                log_info = {
                    'MatchedStudent': student_name,
                    'Timestamp': datetime.now().isoformat(),
                    'ImageKey': image_key
                }
                print(f"Informações do log: {json.dumps(log_info)}")
                
                send_email(student_name, image_key)
                return generate_response(status_code=200, message=f"Bem vindo {student_name}!")
        
        log_info = {
            'MatchedStudent': None,
            'Timestamp': datetime.now().isoformat(),
            'ImageKey': image_key
        }
        print(f"Informações do log: {json.dumps(log_info)}")
        return generate_response(status_code=200, message="Estudante não encontrado!")
    
    except Exception as e:
        print("Erro: " + str(e))
        return generate_response(status_code=500, message=str(e))

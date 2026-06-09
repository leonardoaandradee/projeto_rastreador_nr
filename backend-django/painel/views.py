from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
import json
from .models import MqttUsers

# O seu token secreto (mantenha isso bem guardado nas variáveis de ambiente do Render!)
SECRET_FLEET_TOKEN = "seu-token-super-secreto-aqui"

@csrf_exempt
def register_device(request):
    if request.method == 'POST':
        try:
            data = json.loads(request.body)
            token = data.get('fleet_token')
            username = data.get('username')
            password = data.get('password')

            # 1. Valida o Token Secreto
            if token != SECRET_FLEET_TOKEN:
                return JsonResponse({'status': 'error', 'message': 'Token inválido'}, status=403)

            # 2. Cria o usuário no banco (a tabela MqttUsers que já configuramos)
            if MqttUsers.objects.filter(username=username).exists():
                return JsonResponse({'status': 'error', 'message': 'Usuário já existe'}, status=400)

            MqttUsers.objects.create(username=username, password=password, ativo=True)
            
            return JsonResponse({'status': 'success', 'message': 'Dispositivo registrado com sucesso'})
            
        except Exception as e:
            return JsonResponse({'status': 'error', 'message': str(e)}, status=500)
    
    return JsonResponse({'error': 'Método não permitido'}, status=405)
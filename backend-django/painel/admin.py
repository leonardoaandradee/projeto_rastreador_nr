from django.contrib import admin
from .models import MqttUsers

@admin.register(MqttUsers)
class MqttUsersAdmin(admin.ModelAdmin):
    # Quais colunas vão aparecer na tabela da tela
    list_display = ('username', 'password', 'ativo')
    
    # O Truque Sênior: Permitir que você mude o status de banimento com 1 clique!
    list_editable = ('ativo',)
    
    # Adiciona uma barra de pesquisa para achar o caminhão pelo nome
    search_fields = ('username',)
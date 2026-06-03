from django.db import models

class MqttUsers(models.Model):
    # O Django exige que toda tabela tenha uma chave primária (id)
    id = models.AutoField(primary_key=True)
    username = models.CharField(max_length=100, unique=True)
    password = models.CharField(max_length=100)
    ativo = models.IntegerField(default=1) 

    class Meta:
        managed = True  # O SEGREDO! Impede o Django de bagunçar o XAMPP
        db_table = 'mqtt_users'  # O nome exato da tabela lá no banco de dados
        verbose_name = 'Caminhão'
        verbose_name_plural = 'Caminhões'

    def __str__(self):
        return self.username
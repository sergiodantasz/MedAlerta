from django.contrib.auth.models import User
from django.db import models


class Medicine(models.Model):
    STATUS_CHOICES = [
        ("pending", "pendente"),
        ("taken", "tomado"),
        ("not taken", "não tomado"),
    ]

    name = models.CharField(max_length=50)
    frequency = models.PositiveSmallIntegerField()
    status = models.CharField(max_length=9, choices=STATUS_CHOICES, default="pending")
    user = models.ForeignKey(User, models.CASCADE)

    def __str__(self) -> str:
        return self.name

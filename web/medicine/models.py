from django.contrib.auth.models import User
from django.db import models


class Medicine(models.Model):
    STATUS_CHOICES = [
        ("Pending", "Pendente"),
        ("Taken", "Tomado"),
        ("Not taken", "Não tomado"),
    ]

    name = models.CharField(max_length=50)
    date_and_time = models.DateTimeField()
    status = models.CharField(max_length=9, choices=STATUS_CHOICES, default="Pending")
    user = models.ForeignKey(User, models.CASCADE)

    def get_status_class(self):
        match self.status:
            case "Pending":
                return "pending"
            case "Taken":
                return "taken"
            case "Not taken":
                return "not-taken"

    def __str__(self) -> str:
        return self.name

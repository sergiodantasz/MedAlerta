from django.utils import timezone
from rest_framework import serializers

from medicine.models import Medicine


class MedicineSerializer(serializers.ModelSerializer):
    has_time_reached = serializers.SerializerMethodField()

    class Meta:
        model = Medicine
        fields = ["id", "name", "date_and_time", "status", "has_time_reached"]

    def get_has_time_reached(self, obj):
        return timezone.now() >= obj.date_and_time

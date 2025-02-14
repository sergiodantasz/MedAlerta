from django.contrib import admin

from medicine.models import Medicine


@admin.register(Medicine)
class MedicineAdmin(admin.ModelAdmin):
    list_display = ["id", "name", "date_and_time", "status", "user"]
    list_display_links = ["name"]
    list_editable = ["status"]
    list_per_page = 20
    search_fields = ["name"]
    ordering = ["date_and_time"]

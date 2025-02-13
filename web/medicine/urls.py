from django.urls import path

from . import views

app_name = "medicine"

urlpatterns = [
    path("", views.medicine_list, name="list"),
]

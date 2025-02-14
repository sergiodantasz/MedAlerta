from django.urls import path

from medicine import views

app_name = "medicine"

urlpatterns = [
    path("", views.medicines, name="medicines"),
    path("add/", views.add, name="add"),
    path("remove/<int:id>/", views.remove, name="remove"),
]

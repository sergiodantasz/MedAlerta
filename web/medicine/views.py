from django.contrib.auth.decorators import login_required
from django.contrib.messages import success
from django.db.models import Case, Value, When
from django.shortcuts import get_object_or_404, redirect, render
from django.urls import reverse
from rest_framework.authentication import BasicAuthentication, SessionAuthentication
from rest_framework.decorators import (
    api_view,
    authentication_classes,
    permission_classes,
)
from rest_framework.permissions import IsAuthenticated
from rest_framework.response import Response

from medicine.forms import MedicineForm
from medicine.models import Medicine
from medicine.serializers import MedicineSerializer


@login_required
def medicines(request):
    medicines = (
        Medicine.objects.filter(user=request.user)
        .annotate(
            status_order=Case(
                When(status="Pending", then=Value(0)),
                When(status="Not taken", then=Value(1)),
                When(status="Taken", then=Value(2)),
            )
        )
        .order_by("status_order", "date_and_time")
    )
    context = {
        "medicines": medicines,
    }
    return render(request, "medicine/pages/medicines.html", context)


@login_required
def add(request):
    form = MedicineForm(request.POST or None)
    if request.POST and form.is_valid():
        medicine = form.save(commit=False)
        medicine.user = request.user
        medicine.save()
        success(request, "O medicamento foi adicionado com sucesso.")
        return redirect(reverse("medicine:medicines"))
    context = {
        "form": form,
        "action": reverse("medicine:add"),
    }
    return render(request, "medicine/pages/add.html", context)


@login_required
def remove(request, id):
    medicine = get_object_or_404(Medicine, id=id, user=request.user)
    medicine.delete()
    success(request, "O medicamento foi removido com sucesso.")
    return redirect(reverse("medicine:medicines"))


@api_view(["GET", "POST"])
@authentication_classes([BasicAuthentication, SessionAuthentication])
@permission_classes([IsAuthenticated])
def medicine_api(request):
    if request.method == "GET":
        medicine = (
            Medicine.objects.filter(user=request.user, status="Pending")
            .order_by("date_and_time")
            .first()
        )
        if medicine:
            serializer = MedicineSerializer(medicine)
            return Response(serializer.data)
        return Response({"message": "Nenhum medicamento encontrado."}, status=404)
    elif request.method == "POST":
        medicine_id = request.data.get("id")
        status = request.data.get("status")
        try:
            medicine = Medicine.objects.get(id=medicine_id, user=request.user)
            medicine.status = status
            medicine.save()
            return Response(
                {"message": "Status atualizado com sucesso."},
            )
        except Medicine.DoesNotExist:
            return Response({"error": "Remédio não encontrado"}, status=404)

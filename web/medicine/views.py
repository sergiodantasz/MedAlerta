from django.contrib.auth.decorators import login_required
from django.contrib.messages import success
from django.shortcuts import get_object_or_404, redirect, render
from django.urls import reverse

from medicine.forms import MedicineForm
from medicine.models import Medicine


@login_required
def medicines(request):
    medicines = Medicine.objects.filter(user=request.user).order_by("date_and_time")
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

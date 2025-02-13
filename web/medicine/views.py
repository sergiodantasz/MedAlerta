from django.contrib.auth.decorators import login_required
from django.shortcuts import render

from .models import Medicine


@login_required
def medicine_list(request):
    medicines = Medicine.objects.filter(user=request.user)
    context = {
        "medicines": medicines,
    }
    return render(request, "medicine/pages/medicine_list.html", context)

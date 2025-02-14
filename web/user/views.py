from django.contrib.auth import authenticate
from django.contrib.auth import login as login_
from django.contrib.auth import logout as logout_
from django.contrib.auth.decorators import login_required
from django.contrib.messages import error, success
from django.shortcuts import redirect, render
from django.urls import reverse

from user.forms import LoginForm, RegisterForm


def login(request):
    if request.user.is_authenticated:
        return redirect(reverse("medicine:medicines"))
    form = LoginForm(request.POST or None)
    if request.POST and form.is_valid():
        credentials = {
            "username": form.cleaned_data.get("username"),
            "password": form.cleaned_data.get("password"),
        }
        authenticated_user = authenticate(request, **credentials)
        if not authenticated_user:
            error(request, "As credenciais inseridas não são válidas.")
            return redirect(reverse("user:login"))
        login_(request, authenticated_user)
        success(request, "O login foi realizado com sucesso.")
        return redirect(reverse("medicine:medicines"))
    context = {
        "form": form,
        "action": reverse("user:login"),
    }
    return render(request, "user/pages/login.html", context)


def register(request):
    if request.user.is_authenticated:
        return redirect(reverse("medicine:medicines"))
    form = RegisterForm(request.POST or None)
    if request.POST and form.is_valid():
        user = form.save(commit=False)
        user.set_password(user.password)
        user.save()
        login_(request, user)
        success(request, "O registro foi realizado com sucesso. Você já está logado.")
        return redirect(reverse("medicine:medicines"))
    context = {
        "form": form,
        "action": reverse("user:register"),
    }
    return render(request, "user/pages/register.html", context)


@login_required
def logout(request):
    if not request.POST or request.POST.get("username") != request.user.username:
        error(request, "Ocorreu um erro ao fazer o logout.")
        return redirect(reverse("medicine:medicines"))
    logout_(request)
    success(request, "O logout foi realizado com sucesso.")
    return redirect(reverse("medicine:medicines"))

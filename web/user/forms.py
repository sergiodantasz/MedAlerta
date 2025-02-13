from django import forms
from django.contrib.auth.models import User

from helpers.validators import make_error_messages


class LoginForm(forms.Form):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.fields["username"].widget.attrs["class"] = "form-field"
        self.fields["password"].widget.attrs["class"] = "form-field"

    username = forms.CharField(
        label="Nome de usuário",
        required=True,
        min_length=5,
        max_length=30,
    )
    password = forms.CharField(
        label="Senha",
        widget=forms.PasswordInput(),
        required=True,
        min_length=8,
    )


class RegisterForm(forms.ModelForm):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.fields["first_name"].widget.attrs["class"] = "form-field"
        self.fields["last_name"].widget.attrs["class"] = "form-field"
        self.fields["username"].widget.attrs["class"] = "form-field"
        self.fields["password"].widget.attrs["class"] = "form-field"

    class Meta:
        model = User
        fields = ["first_name", "last_name", "username", "password"]

    first_name = forms.CharField(
        label="Primeiro nome",
        required=True,
        min_length=1,
        max_length=30,
        error_messages=make_error_messages(
            ["unique"],
            field="primeiro nome",
            min_length=1,
            max_length=30,
        ),
    )
    last_name = forms.CharField(
        label="Último nome",
        required=True,
        min_length=1,
        max_length=30,
        error_messages=make_error_messages(
            ["unique"],
            field="último nome",
            min_length=1,
            max_length=30,
        ),
    )
    username = forms.CharField(
        label="Nome de usuário",
        required=True,
        min_length=5,
        max_length=30,
        error_messages=make_error_messages(
            field="nome de usuário",
            min_length=5,
            max_length=30,
        ),
    )
    password = forms.CharField(
        label="Senha",
        widget=forms.PasswordInput(),
        required=True,
        min_length=8,
        error_messages=make_error_messages(
            ["unique", "max_length"],
            field="senha",
            min_length=1,
            max_length=30,
        ),
    )

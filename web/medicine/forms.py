from django import forms

from helpers.validators import make_error_messages
from medicine.models import Medicine


class MedicineForm(forms.ModelForm):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.fields["name"].widget.attrs["class"] = "form-field"
        self.fields["date_and_time"].widget.attrs["class"] = "form-field"

    class Meta:
        model = Medicine
        fields = ["name", "date_and_time"]

    name = forms.CharField(
        label="Nome",
        required=True,
        min_length=1,
        max_length=50,
        error_messages=make_error_messages(
            ["unique"],
            field="nome",
            min_length=1,
            max_length=50,
        ),
    )
    date_and_time = forms.DateTimeField(
        label="Data e hora",
        widget=forms.DateTimeInput(attrs={"type": "datetime-local"}),
        required=True,
        error_messages=make_error_messages(
            ["unique", "min_length", "max_length"],
            field="data e hora",
        ),
    )

#!/bin/bash

python3.12 -m venv .venv
source ./.venv/bin/activate

pip install --upgrade pip
pip install -r ./requirements.txt

python manage.py tailwind install
python manage.py tailwind build

python manage.py makemigrations --no-input
python manage.py migrate --no-input
python manage.py collectstatic --no-input --clear

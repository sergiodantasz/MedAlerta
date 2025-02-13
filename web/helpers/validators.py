ERROR_MESSAGES = {
    "required": "O {field} é obrigatório.",
    "min_length": "O {field} deve ter pelo menos {min_length} caracteres.",
    "max_length": "O {field} deve ter no máximo {max_length} caracteres.",
    "invalid": "O {field} não é válido.",
    "unique": "O {field} já existe.",
}


def make_error_messages(exceptions=None, **constraints):
    """Makes custom error messages for a form field.

    Args:
        exceptions (list | tuple | None, optional): the exceptions of the constraints (field, min_length, max_length). Defaults to None.
        constraints: the values that will populate the error messages. It can be "field", "min_length" and "max_length".

    Returns:
        dict[str, str]: key and value pairs containing the type of error and its message.
    """
    if not isinstance(exceptions, (list, tuple)) and exceptions is not None:
        raise TypeError("The exceptions must be a list or tuple instance or None.")
    if not exceptions:
        exceptions = []
    errors = {}
    for error_type, error_value in ERROR_MESSAGES.items():
        if error_type in exceptions:
            continue
        errors[error_type] = error_value.format(**constraints)
    return errors

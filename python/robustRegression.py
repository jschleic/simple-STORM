import rpy2.robjects as robjects

# load robust statistics R library
robjects.r.library("robustbase")



def lmrob(x,y, model = 'y ~ x'):
    ''' 
    Robust regression.

    Fits the model in a robust way using a MM-regression.
    
    model A model in R notation.

    Returns a dictionary with the keys:
    - 'a'
    - 'b'
    - 'Std. Dev. a'
    - 'Std. Dev. b'
    - 'Pr(a==0)' - Probability, that a equals zero
    - 'Pr(b==0)' - Probability, that b equals zero
    - 'Weights' - Outliers get zero weight
    - 'Summary' - Detailed information as a printable string
    '''        
    x_R = robjects.FloatVector(x)
    y_R = robjects.FloatVector(y)

    model_R = robjects.RFormula(model)
    env_R = model_R.getenvironment()
    env_R['x'] = x_R
    env_R['y'] = y_R

    fit_R = robjects.r.lmrob(model_R)

    summary_R = robjects.r.summary(fit_R)
    coefficients_R = summary_R.subset('coefficients')

    b = float(coefficients_R[0][0])
    a = float(coefficients_R[0][1])
    error_b = float(coefficients_R[0][2])
    error_a = float(coefficients_R[0][3])
    zero_prob_b = float(coefficients_R[0][6])
    zero_prob_a = float(coefficients_R[0][7])
    weights = list(summary_R.subset('weights')[0])

    return {'a': a, 
            'b': b, 
            'Std. Dev. a': error_a, 
            'Std. Dev. b': error_b, 
            'Pr(a==0)': zero_prob_a, 
            'Pr(b==0)': zero_prob_b,
            'Weights': weights,
            'Summary': str(summary_R)}


if __name__ == "__main__":
    testdata = [\
    (50,0.44),
    (51,0.47),
    (52,0.47),
    (53,0.59),
    (54,0.66),
    (55,0.73),
    (56,0.81),
    (57,0.88),
    (58,1.06),
    (59,1.20),
    (60,1.35),
    (61,1.49),
    (62,1.61),
    (63,2.12),
    (64,11.90),
    (65,12.40),
    (66,14.20),
    (67,15.90),
    (68,18.20),
    (69,21.20),
    (70,4.30),
    (71,2.40),
    (72,2.70),
    (73,2.90)
    ]

    x = robjects.FloatVector(zip(*testdata)[0])
    y = robjects.FloatVector(zip(*testdata)[1])

    fmla = robjects.RFormula('y ~ x')
    env = fmla.getenvironment()
    env['x'] = x
    env['y'] = y

    fit_orig = robjects.r.lmrob(fmla)
    fit_py = lmrob(zip(*testdata)[0], zip(*testdata)[1])

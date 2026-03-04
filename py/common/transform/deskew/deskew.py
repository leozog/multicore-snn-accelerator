# This code was adapted from https://fsix.github.io/mnist/Deskewing.html
# and is licensed under the Apache License, Version 2.0.

import numpy as np
from scipy.ndimage import interpolation

class Deskew:
    def _moments(self, image):
        c0, c1 = np.mgrid[:image.shape[0], :image.shape[1]]
        totalImage = np.sum(image)
        m0 = np.sum(c0 * image) / totalImage
        m1 = np.sum(c1 * image) / totalImage
        m00 = np.sum((c0 - m0) ** 2 * image) / totalImage
        m11 = np.sum((c1 - m1) ** 2 * image) / totalImage
        m01 = np.sum((c0 - m0) * (c1 - m1) * image) / totalImage
        mu_vector = np.array([m0, m1])
        covariance_matrix = np.array([[m00, m01], [m01, m11]])
        return mu_vector, covariance_matrix

    def __call__(self, image):
        image = np.array(image)
        c, v = self._moments(image)
        alpha = v[0, 1] / v[0, 0]
        affine = np.array([[1, 0], [alpha, 1]])
        ocenter = np.array(image.shape) / 2.0
        offset = c - np.dot(affine, ocenter)
        return interpolation.affine_transform(image, affine, offset=offset)

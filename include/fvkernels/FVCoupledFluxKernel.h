// Nicolò Grilli
// University of Bristol
// 5 Luglio 2021

#pragma once

#include "FVKernel.h"
#include "FVUtils.h"
#include "NeighborCoupleable.h"
#include "TwoMaterialPropertyInterface.h"
#include "NeighborMooseVariableInterface.h"
#include "NeighborCoupleableMooseVariableDependencyIntermediateInterface.h"

class FaceInfo;

/// FVCoupledFluxKernel is used for calculating residual contributions from numerical
/// fluxes of a coupled variable from surface integral terms 
/// in a finite volume discretization of a
/// PDE (i.e.  terms where the divergence theorem is applied).  As with finite
/// element kernels, all solution values and material properties must be
/// indexed using the _qp member.  Note that all interfaces for finite volume
/// kernels are AD-based - be sure to use AD material properties and other AD
/// values to maintain good jacobian/derivative quality.
class FVCoupledFluxKernel : public FVKernel,
                     public TwoMaterialPropertyInterface,
                     public NeighborMooseVariableInterface<Real>,
                     public NeighborCoupleableMooseVariableDependencyIntermediateInterface
{
public:
  static InputParameters validParams();
  FVCoupledFluxKernel(const InputParameters & params);

  /// Usually you should not override these functions - they have some super
  /// tricky stuff in them that you don't want to mess up!
  // @{
  virtual void computeResidual(const FaceInfo & fi);
  virtual void computeJacobian(const FaceInfo & fi);
  /// @}

protected:
  /// This is the primary function that must be implemented for flux kernel
  /// terms.  Material properties will be initialized on the face - using any
  /// reconstructed fv variable gradients if any.  Values for the solution are
  /// provided for both the elem and neighbor side of the face.
  virtual ADReal computeQpResidual() = 0;

  /// Calculates and returns "grad_u dot normal" on the face to be used for
  /// diffusive terms.  If using any cross-diffusion corrections, etc. all
  /// those calculations will be handled for appropriately by this function.
  virtual ADReal gradUDotNormal() const;

  const ADRealVectorValue & normal() const { return _normal; }

  MooseVariableFV<Real> & _u_var; // u variable

  MooseVariableFV<Real> & _var; // coupled variable

  const unsigned int _qp = 0;

  /// The elem solution value of the kernel's _var for the current face.
  const ADVariableValue & _u_elem;
  /// The neighbor solution value of the kernel's _var for the current face.
  const ADVariableValue & _u_neighbor;
  /// The elem solution gradient of the kernel's _var for the current face.
  /// This is zero unless higher order reconstruction is used.
  const ADVariableGradient & _grad_u_elem;
  /// The neighbor solution gradient of the kernel's _var for the current face.
  /// This is zero unless higher order reconstruction is used.
  const ADVariableGradient & _grad_u_neighbor;

  /// This is the outward unit normal vector for the face the kernel is currently
  /// operating on.  By convention, this is set to be pointing outward from the
  /// face's elem element and residual calculations should keep this in mind.
  ADRealVectorValue _normal;

  /// This is holds meta-data for geometric information relevant to the current
  /// face including elem+neighbor cell centroids, cell volumes, face area, etc.
  const FaceInfo * _face_info = nullptr;

private:
  /// Computes the Jacobian contribution for every coupled variable.
  ///
  /// @param type Either ElementElement, ElementNeighbor, NeighborElement, or NeighborNeighbor. As an
  /// example ElementNeighbor means the derivatives of the elemental residual with respect to the
  /// neighbor degrees of freedom.
  ///
  /// @param residual The already computed residual (probably done with \p computeQpResidual) that
  /// also holds derivative information for filling in the Jacobians.
  void computeJacobian(Moose::DGJacobianType type, const ADReal & residual);

  /// Kernels are called even on boundaries in case one is for a variable with
  /// a dirichlet BC - in which case we need to run the kernel with a
  /// ghost-element.  This returns true if we need to run because of dirichlet
  /// conditions - otherwise this returns false and all jacobian/residual calcs
  /// should be skipped.
  bool skipForBoundary(const FaceInfo & fi);

  const bool _force_boundary_execution;
};

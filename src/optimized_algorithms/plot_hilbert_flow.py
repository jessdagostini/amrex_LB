import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d.art3d import Line3DCollection
import matplotlib.colors as colors

def transform(pos, orientation, n):
    """Transform coordinates to maintain face-to-face connections."""
    x, y, z = pos
    m = n - 1  # Pre-calculate n-1 for efficiency
    
    # Transformation rules that ensure proper connectivity between subcubes
    transformations = {
        0: lambda: [x, y, z],           # identity
        1: lambda: [z, y, x],           # rotate around y=z diagonal
        2: lambda: [y, z, x],           # rotate around x=z diagonal
        3: lambda: [m-y, z, x],         # rotate and reflect
        4: lambda: [m-z, y, x],         # rotate around x axis
        5: lambda: [m-x, y, z],         # reflect in x
        6: lambda: [x, m-y, z],         # reflect in y
        7: lambda: [x, y, m-z],         # reflect in z
    }
    
    return np.array(transformations.get(orientation, transformations[0])())

def hilbert_curve(n):
    """Generate points for a 3D Hilbert curve."""
    if n == 1:
        return [np.array([0, 0, 0])]
    
    n2 = n // 2  # half size
    
    # Define subcube configurations to ensure proper connectivity
    # Each subcube connects to the next through a shared face
    subcube_configs = [
        [0, 0, 0, 0],      # start at origin (lower left back)
        [0, 0, n2, 1],     # connect through back face
        [0, n2, n2, 2],    # connect through top back
        [0, n2, 0, 3],     # connect through top front
        [n2, n2, 0, 6],    # connect through right top front
        [n2, n2, n2, 4],   # connect through right top back
        [n2, 0, n2, 5],    # connect through right bottom back
        [n2, 0, 0, 7],     # finish at right bottom front
    ]
    
    subcurves = []
    for x, y, z, orientation in subcube_configs:
        # Generate base subcurve
        subcurve = hilbert_curve(n2)
        
        # Transform and position each point
        transformed = []
        for point in subcurve:
            # Apply orientation transformation
            new_point = transform(point, orientation, n2)
            # Translate to correct position
            new_point = new_point + np.array([x, y, z])
            transformed.append(new_point)
        
        subcurves.extend(transformed)
    
    return subcurves

def hilbert3d(order):
    """Generate 3D Hilbert curve coordinates for the given order."""
    if order < 1:
        raise ValueError("Order must be at least 1")
    
    # Calculate the size of the curve (2^order)
    n = 2 ** order
    
    # Generate the curve points
    points = hilbert_curve(n)
    
    # Convert to numpy array for easier manipulation
    return np.array(points)

def plot_hilbert_flow(order, view_angles=None):
    """Plot the Hilbert curve flow with colors and arrows."""
    points = hilbert3d(order)
    
    if view_angles is None:
        view_angles = [
            (45, 45),    # Isometric view
            (0, 0),      # Front view
            (0, 90),     # Side view
            (90, 0),     # Top view
        ]
    
    n_views = len(view_angles)
    fig = plt.figure(figsize=(5*n_views, 5))
    
    for idx, (elev, azim) in enumerate(view_angles, 1):
        ax = fig.add_subplot(1, n_views, idx, projection='3d')
        
        # Create line segments
        segments = np.concatenate([points[:-1, None, :], points[1:, None, :]], axis=1)
        
        # Create color gradient along the curve
        t = np.linspace(0, 1, len(points)-1)
        colors_array = plt.cm.viridis(t)
        
        # Plot line segments with color gradient
        lc = Line3DCollection(segments, colors=colors_array)
        ax.add_collection3d(lc)
        
        # Plot points
        scatter = ax.scatter(points[:, 0], points[:, 1], points[:, 2],
                           c=np.linspace(0, 1, len(points)),
                           cmap='viridis', s=20)
        
        # Add arrows at regular intervals to show direction
        arrow_indices = np.linspace(0, len(points)-2, min(20, len(points)-1), dtype=int)
        for i in arrow_indices:
            p1, p2 = points[i], points[i+1]
            direction = p2 - p1
            midpoint = (p1 + p2) / 2
            ax.quiver(midpoint[0], midpoint[1], midpoint[2],
                     direction[0], direction[1], direction[2],
                     color='red', length=0.1, normalize=True)
        
        # Mark start and end points
        ax.scatter(*points[0], c='green', s=100, label='Start', marker='*')
        ax.scatter(*points[-1], c='red', s=100, label='End', marker='*')
        
        # Set labels and title
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        ax.set_zlabel('Z')
        
        # Dynamic view names based on angles
        if len(view_angles) == 4:  # Default views
            view_names = ['Isometric', 'Front', 'Side', 'Top']
            title = f'{view_names[idx-1]} View'
        else:  # Detailed rotation views
            title = f'View (elev={elev}°, azim={azim}°)'
        ax.set_title(title)
        
        # Set viewing angle
        ax.view_init(elev=elev, azim=azim)
        
        # Set equal aspect ratio
        ax.set_box_aspect([1,1,1])
        
        # Add legend
        ax.legend()
        
        # Add colorbar
        if idx == n_views:
            plt.colorbar(scatter, ax=ax, label='Position along curve')
    
    plt.tight_layout()
    return fig

def main():
    # Generate visualizations for different orders
    for order in [1, 2, 3]:
        fig = plot_hilbert_flow(order)
        plt.savefig(f'hilbert_flow_order{order}.png', dpi=300, bbox_inches='tight')
        plt.close()
        
        # Create additional views for detailed analysis
        detailed_views = [
            [(45, angle) for angle in range(0, 360, 45)],  # Rotate around
            [(angle, 45) for angle in range(0, 360, 45)]   # Tilt
        ]
        
        for view_type, views in enumerate(detailed_views):
            fig = plot_hilbert_flow(order, views)
            plt.savefig(f'hilbert_flow_order{order}_detail{view_type}.png',
                       dpi=300, bbox_inches='tight')
            plt.close()

if __name__ == "__main__":
    main()
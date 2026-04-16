import random
import string
import os
from reportlab.lib.pagesizes import letter
from reportlab.pdfgen import canvas

def generate_random_text(length):
    """Generate a random string of given length."""
    return ''.join(random.choices(string.ascii_letters + string.digits + string.punctuation + string.whitespace, k=length))

def create_random_pdf(file_path, min_pages, max_pages, min_chars, max_chars):
    """Create a random PDF file with random pages and characters using reportlab."""
    num_pages = random.randint(min_pages, max_pages)
    c = canvas.Canvas(file_path, pagesize=letter)

    for _ in range(num_pages):
        text_length = random.randint(min_chars, max_chars)
        random_text = generate_random_text(text_length)
        
        # Add random text to the page
        c.setFont("Helvetica", 12)
        lines = random_text.split("\n")
        y = 750  # Start near the top of the page
        for line in lines:
            c.drawString(50, y, line[:100])  # Limit line length to 100 characters
            y -= 15  # Move down for the next line
            if y < 50:  # If text reaches the bottom, move to a new page
                c.showPage()
                c.setFont("Helvetica", 12)
                y = 750
        
        # End the current page
        c.showPage()
    
    # Save the PDF
    c.save()

def generate_multiple_random_pdfs(output_dir, num_files, min_pages, max_pages, min_chars, max_chars):
    """Generate multiple random PDF files with random pages and characters."""
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    for i in range(num_files):
        file_name = f"random_pdf_{i + 1}.pdf"
        file_path = os.path.join(output_dir, file_name)
        create_random_pdf(file_path, min_pages, max_pages, min_chars, max_chars)
        print(f"Generated: {file_path}")

# Configuration
output_directory = "random_pdfs"
number_of_files = 10
min_pages = 1
max_pages = 10
min_characters = 500
max_characters = 2000

# Generate the random PDF files
generate_multiple_random_pdfs(output_directory, number_of_files, min_pages, max_pages, min_characters, max_characters)
